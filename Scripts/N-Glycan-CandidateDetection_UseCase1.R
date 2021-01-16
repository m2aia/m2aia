
library("Cardinal")

args <- commandArgs(trailingOnly = TRUE)
print(args)
pngAll <- Cardinal::readMSIData(args[1])

centroided(pngAll) = TRUE

pngMean <- summarizeFeatures(pngAll, "mean")
png(filename=paste(args[2],"InputFeatures.png",sep=""))
plot(pngMean,grid=TRUE, type="h")
dev.off()

pcaObj <- PCA(pngAll, ncomp=3)
png(filename=paste(args[2],"PCA_result.png",sep=""))
image(pcaObj, contrast.enhance="histogram", normalize.image="linear",superpose=TRUE)
dev.off()

# label pixels
sampleA = as.logical(c(rep(TRUE,9179),rep(FALSE,1770)))
sampleB = as.logical(c(rep(FALSE,9179),rep(TRUE,1770)))
regions <- makeFactor(A=sampleA, B=sampleB)


res <- spatialKMeans(pngAll, r=1, k=4, method="adaptive" )
png(filename=paste(args[2],"KMeans_result.png",sep=""))
image(res)
dev.off()

ssc_res <- spatialShrunkenCentroids(pngAll, y=regions, method="adaptive", r=2, s=0)
png(filename=paste(args[2],"SSC_result.png",sep=""))
image(ssc_res)
dev.off()

write.csv(x = topFeatures(ssc_res, model=list(s=0), class == "A",n = 100),file = paste(args[2],"Candidates.csv", sep=""))
write.csv(x = topFeatures(ssc_res, model=list(s=0), class == "B", n = 100),file = paste(args[2],"NoCandidates.csv", sep=""))
