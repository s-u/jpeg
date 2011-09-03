\name{writeJPEG}
\alias{writeJPEG}
\title{
Write a bitmap image in JPEG format
}
\description{
Create a JPEG image from an array or matrix.
}
\usage{
writeJPEG(image, target, quality = 0.7, bg = "white")
}
\arguments{
  \item{image}{image represented by a real matrix or array with values
    in the range of 0 to 1. Values outside this range will be
    clipped. The object must be either two-dimensional (grayscale
    matrix) or three dimensional array (third dimension specifying the
    plane) and must have either one (grayscale), two (grayscale +
    alpha), three (RGB) or four (RGB + alpha) planes. (For alternative
    image specifications see deatils)}
  \item{target}{Either name of the file to write to or a raw vector
    (\code{raw()} is good enough) indicating that the output should be a
    raw vector.}
  \item{quality}{JPEG quality - a real number between 0 (lowest) and 1
    (highest) controlling the quality of the output. Lower quality
    produces smaller, but more lossy files.}
  \item{bg}{background color - used only if the input contains alpha
    channel since JPEG does not support storage of the alpha channel and
    thus the image needs to be flattened as if it was placed over the
    background of this color.}
}
\value{
  Either NULL if the target is a file or a raw vector containing the
  compressed JPEG image if the target was a raw vector.
}
\details{
  \code{writeJPEG} takes an image as input and compresses it into JPEG
  format. The image input is usually a matrix (for grayscale images -
  dimensions are width, height) or an array (for color and alpha
  images - dimensions are width, height, planes) of reals. The planes
  are interpreted in the sequence red, green, blue. For convenience
  \code{writeJPEG} allows the source to include alpha channel, but JEPG
  doe NOT support alpha so it will be discarded.

  Alternative representation of an image is of \code{nativeRaster} class
  which is an integer matrix with each entry representing one pixel in
  binary encoded RGBA format (as used internally by R). It can be
  obtained from \code{\link{readJPEG}} using \code{native = TRUE}.

  Finally, \code{writeJPEG} also supports raw array containing the RGBA
  image as bytes. The dimensions of the raw array have to be planes,
  width, height (because the storage is interleaved). Currently only 4
  planes (RGBA) are supported and the processing is equivalent to that
  of a native raster.

  The result is either stored in a file (if \code{target} is a file
  name) or in a raw vector (if \code{target} is a raw vector).
}
%\references{
%}
%\author{
%}
\note{
  Currently \code{writeJPEG} only produces 8-bit, deflate-compressed,
  non-quantized, non-interlaced images. Note in particular that
  \code{\link{readJPEG}} can read 16-bit channels but storing them
  back using \code{writeJPEG} will strip the 8 LSB (irrelevant for
  display purposes but possibly relevant for use of JPEG in
  signal-processing if the input is truly 16-bit wide).
}
\seealso{
\code{\link{readJPEG}}
}
\examples{
# read a sample file (R logo)
img <- readJPEG(system.file("img","Rlogo.jpg",package="jpeg"))
# write the image into a raw vector - using a low quality
r <- writeJPEG(img, raw(), quality=0.3)
# read it back again
img2 <- readJPEG(r)
# it will be different
identical(img, img2)
# try to write a native raster
img3 <- readJPEG(system.file("img","Rlogo.jpg",package="jpeg"), TRUE)
r2 <- writeJPEG(img3, raw())
img4 <- readJPEG(r2, TRUE)
identical(img3, img4)
}
\keyword{IO}
