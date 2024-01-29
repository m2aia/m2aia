import argparse
import numpy as np
parser = argparse.ArgumentParser(prog="simCLR", 
                                 description="Running simCLR!",
                                 epilog="Original work by Hu, Hang, Jyothsna Padmakumar Bindu, and Julia Laskin. “Self-Supervised Clustering of Mass Spectrometry Imaging Data Using Contrastive Learning.” Chemical Science 13, no. 1 (2022): 90–98. https://doi.org/10.1039/D1SC04077D.")

# parser.add_argument = add_qt_widget(parser.add_argument)

parser.add_argument("--normalization", default="None")
parser.add_argument("--intensity_transform", default="None" )
parser.add_argument("--range_pooling", default="Mean")
parser.add_argument("--smoothing", default="None")
parser.add_argument("--baseline_correction", default="None")
parser.add_argument("--smoothing_value", default=2, type=np.int32)
parser.add_argument("--baseline_correction_value", default=50, type=np.int32)

parser.add_argument("-i", "--imzml", required=True)
parser.add_argument("-c", "--centroids", required=True)
parser.add_argument("-u", "--umap", required=True)
parser.add_argument("-o", "--out", required=True)
parser.add_argument("-m", "--model", default="/tmp/model.simclr", required=False)
parser.add_argument("-e", "--epochs", default=50, type=np.int32)
parser.add_argument("-t", "--tolerance", default=75, type=np.float32)
parser.add_argument("-nc", "--num_clusters", default=10, type=np.int32)
parser.add_argument("-nn", "--num_neighbors", default=10, type=np.int32)
parser.add_argument("-b", "--batch_size", default=64, type=np.int32)

args = parser.parse_args()



import warnings
from numba.core.errors import NumbaDeprecationWarning, NumbaPendingDeprecationWarning
warnings.simplefilter('ignore', category=NumbaDeprecationWarning)
warnings.simplefilter('ignore', category=NumbaPendingDeprecationWarning)

from pathlib import Path
import os
import pandas as pd
import m2aia as m2
import umap
from sklearn import cluster
import seaborn as sea
import torch
from torch import nn
from torchvision import transforms
from torch.utils.data import Dataset, DataLoader, SubsetRandomSampler
from torch.optim.lr_scheduler import CosineAnnealingLR
from torch.optim import Adam
import matplotlib.pyplot as plt 
from SimCLR.code.models import CLR, ContrastiveLoss
import SimpleITK as sitk


# figures
max_dim = 0
img_size = 124
cols = 15
# clustering
n_clusters = 10
n_neighbors = 10
# training
batch_size = 64

I = m2.ImzMLReader(args.imzml)
I.SetSmoothing(args.smoothing, args.smoothing_value)
I.SetBaselineCorrection(args.baseline_correction, args.baseline_correction_value)
I.SetNormalization(args.normalization)
I.SetPooling(args.range_pooling)
I.SetIntensityTransformation(args.intensity_transform)
I.Load()

max_dim = np.max([np.max(I.GetShape()[:2]),max_dim])

#===================================
# 1.0 LoadCentroids
#===================================
data = np.genfromtxt(args.centroids,skip_header=1, delimiter=',',dtype=np.float32)
centroids = data[:,0]

print(centroids)

#===================================
# 1.0 Initialize the ion image Dataset
#===================================
trans = transforms.Compose([transforms.Lambda(lambda x : torch.Tensor(np.clip(x / np.quantile(x,0.999),0,1))),
                            transforms.CenterCrop(max_dim),
                            transforms.Resize(img_size)])
dataset = m2.IonImageDataset([I], 
                            centroids=centroids, 
                            tolerance=args.tolerance, 
                            tolerance_type='ppm', 
                            buffer_type='memory', 
                            transforms=trans)




#===================================
# 1.3 Initialize the augmented ion image 
# Dataset which is used during training
#===================================

# adaptions to gaussian noise to work on single channeled images
def gaussian_noise(pix, mean=0, sigmas=(0.001, 0.2)):
    sigma = np.random.uniform(sigmas[0], sigmas[1])   # randomize magnitude
    pix = pix * 255
    # adaptively tune the magnitude, hardcode according to the data distribution. every img is [0, 255]
    if pix[pix > 25].shape[0] > 0:       # 1st thre 25
        aver = torch.mean(pix[pix > 25])
    elif pix[pix > 20].shape[0] > 0:     # 2nd thre 20
        aver = torch.mean(pix[pix > 20])
    elif pix[pix > 15].shape[0] > 0:     # 3nd thre 15
        aver = torch.mean(pix[pix > 15])
    elif pix[pix > 10].shape[0] > 0:     # 4nd thre 10
        aver = torch.mean(pix[pix > 10])
    else:
        aver = torch.mean(pix)
        
    sigma_adp = sigma/153*aver           # 153, 0 homogeneous img average pixel intensity

    # scale gray img to [0, 1]
    pix = pix / 255
    # generate gaussian noise
    noise = np.random.normal(mean, sigma_adp, pix.shape)
    # generate image with gaussian noise
    pix_out = pix + torch.tensor(noise)
    pix_out = np.clip(pix_out, 0, 1)
    img_out = pix_out # convert to PIL image
    return torch.as_tensor(img_out, dtype=torch.float32)

# Dataset returns two augmentations of the same ion image Dataset entry
class AugmentedDataset(Dataset):
  def __init__(self, dataset: m2.Dataset, transform_f):
    super().__init__()
    self.dataset = dataset
    self.transform_f = transform_f

  def __len__(self):
    return self.dataset.__len__()

  def __getitem__(self, index):
    I = self.dataset[index]
    X = self.transform_f(I)
    Y = self.transform_f(I)
    return X, Y

blur_kernel_size = 9
augmentations = transforms.Compose([transforms.Lambda(lambda x: torch.tensor(x, dtype=torch.float32)),
                                    transforms.GaussianBlur(blur_kernel_size, sigma=(0.01, 0.75)), 
                                    transforms.ColorJitter(brightness=0.5, contrast=0.5, saturation=0.5, hue=0.2), 
                                    transforms.Lambda(gaussian_noise)])
aug_dataset = AugmentedDataset(dataset, augmentations)

#===================================
# 2 Initializing the SimCLR model
#===================================
m = CLR()
m = m.cuda()

#===================================
# 3 Load trained model parameters
#===================================
if os.path.exists(args.model):
    state_dict = torch.load(args.model)
    m.load_state_dict(state_dict)

#===================================
# 4 Start finetuning and save model
#===================================

loss = ContrastiveLoss(batch_size)
loss = loss.cuda()

optim = Adam(m.parameters())
scheduler = CosineAnnealingLR(optim, args.epochs)

dataloader = DataLoader(aug_dataset,
                        batch_size = batch_size, 
                        sampler = SubsetRandomSampler(list(range(len(aug_dataset)))),
                        pin_memory = True,
                        drop_last = True)

m.train()
## main fit steps
total_losses = []
for epoch in range(args.epochs):
    epoch_losses = []
    for i, [X, Y] in enumerate(dataloader):
        X = X.cuda(non_blocking=True)
        Y = Y.cuda(non_blocking=True)
        _, proj_X = m(X)
        _, proj_Y = m(Y)
        loss_value = loss(proj_X, proj_Y)
        epoch_losses.append(loss_value)
        
        # backward
        optim.zero_grad()
        loss_value.backward()
        optim.step()

    # update lr
    scheduler.step()
    losses_np = torch.tensor(epoch_losses).cpu().numpy()
    print('epoch {} loss: {}'.format(epoch, np.mean(losses_np)))

torch.save(m.state_dict(),  args.model)


#===================================
# 3.1 Predict embeddings of the images
# using the finetuned model
#===================================

m.eval()
hatA = None
for ionImage in dataset:
    # Dataset returns item of form [C,H,W]
    # Network requires items of form [N,C,H,W]
    _, embedding = m(torch.tensor(ionImage[None,...]).cuda())
    embedding = embedding.cpu().detach().numpy()
    if hatA is None:
        hatA = embedding
    else:
        hatA = np.concatenate([hatA, embedding])

print("""
#===================================
# 3.2 Cluster the results using SpectralCustering
#===================================
""")
predictor = cluster.SpectralClustering(random_state=42,
                                n_clusters=n_clusters, 
                                n_neighbors = n_neighbors,
                                affinity = 'nearest_neighbors', 
                                assign_labels='discretize')
clusteredHatA = predictor.fit_predict(hatA)

#===================================
# 3.3 Use UMAP embeddings
#===================================
transformer = umap.UMAP(random_state=65)
transformedHatA = transformer.fit_transform(hatA)
targetCluster = 5

sea.set_style('darkgrid')
plt.figure(figsize=(8,8))
fig = sea.scatterplot(x=transformedHatA[:,0], y=transformedHatA[:,1], hue=clusteredHatA ,legend=False, palette='colorblind')
plt.savefig(args.umap)

ys = I.GetMeanSpectrum()
xs = I.GetXAxis()

with open(args.out, mode="w") as f:
    f.write("center,intensity,label\n")
    for center, label in zip(centroids, clusteredHatA):
        ys_i = np.argmin(np.abs(xs-center))
        f.write(f"{center},{ys[ys_i]},{label}\n")
        print(f"{center},{ys[ys_i]},{label}\n")

print("Done!")
