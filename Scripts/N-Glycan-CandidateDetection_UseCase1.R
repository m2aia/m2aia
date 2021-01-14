
library("Cardinal")
pngAll <- Cardinal::readMSIData("/data/combi_result.imzML")

centroided(pngAll) = TRUE

pngMean <- summarizeFeatures(pngAll, "mean")
png(filename="/results/InputFeatures.png")
plot(pngMean,grid=TRUE, type="h")
dev.off()

pcaObj <- PCA(pngAll, ncomp=3)
png(filename="/results/PCA_result.png")
image(pcaObj, contrast.enhance="histogram", normalize.image="linear",superpose=TRUE)
dev.off()

# label pixels
sampleA = as.logical(c(rep(TRUE,9179),rep(FALSE,1770)))
sampleB = as.logical(c(rep(FALSE,9179),rep(TRUE,1770)))
regions <- makeFactor(A=sampleA, B=sampleB)


res <- spatialKMeans(pngAll, r=1, k=4, method="adaptive" )
png(filename="/results/KMeans_result.png")
image(res)
dev.off()

ssc_res <- spatialShrunkenCentroids(pngAll, y=regions, method="adaptive", r=2, s=0)
png(filename="/results/SSC_result.png")
image(ssc_res)
dev.off()

// write as csv
topFeatures(ssc_res, model=list(s=0), class == "A",n = 100)
topFeatures(ssc_res, model=list(s=0), class == "B", n = 100)
