
library("Cardinal")
pngAll <- Cardinal::readMSIData("D:\\HSMannheim/Data/MFoell/M2aind_201116/combi_result.imzML")

centroided(pngAll) = TRUE
#pngAll %>% mzBin() %>% process() 

pngMean <- summarizeFeatures(pngAll, "mean")
plot(pngMean,grid=TRUE, type="h")

pcaObj <- PCA(pngAll, ncomp=3)
image(pcaObj, contrast.enhance="histogram", normalize.image="linear",superpose=TRUE)

# label pixels
sampleA = as.logical(c(rep(TRUE,9179),rep(FALSE,1770)))
sampleB = as.logical(c(rep(FALSE,9179),rep(TRUE,1770)))
regions <- makeFactor(A=sampleA, B=sampleB)


res <- spatialKMeans(pngAll, r=1, k=4, method="adaptive" )
summary(res)
image(res)

ssc_res <- spatialShrunkenCentroids(pngAll, y=regions, method="adaptive", r=2, s=0)
#summary(ssc_res)
image(ssc_res)

#plot(ssc_res, model=list(s=0), values="statistic", lwd=2)
topFeatures(ssc_res, model=list(s=0), class == "A",n = 100)
topFeatures(ssc_res, model=list(s=0), class == "B", n = 100)
