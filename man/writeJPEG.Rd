\name{writeJPEG}
\alias{writeJPEG}
\title{
Write a bitmap image in JPEG format
}
\description{
Create a JPEG image from an array or matrix.
}
\usage{
writeJPEG(image, target = raw(), quality = 0.7, bg = "white", color.space)
}
\arguments{
  \item{image}{image represented by a real matrix or array with values
    in the range of 0 to 1. Values outside this range will be
    clipped. The object must be either two-dimensional (grayscale
    matrix) or three dimensional array (third dimension specifying the
    plane) and must have either one (grayscale), two (grayscale +
    alpha), three (RGB) or four (RGB + alpha) planes. (For alternative
    image specifications see deatils)}
  \item{target}{Either name of the file to write to, or a binary
    connection, or a raw vector (\code{raw()} - the default - is good
    enough) indicating that the output should be a raw vector.}
  \item{quality}{JPEG quality - a real number between 0 (lowest) and 1
    (highest) controlling the quality of the output. Lower quality
    produces smaller, but more lossy files.}
  \item{bg}{background color - used only if the input contains alpha
    channel since JPEG does not support storage of the alpha channel and
    thus the image needs to be flattened as if it was placed over the
    background of this color.}
  \item{color.space}{color space in which the image data is to be
    interpreted. Defaults to the \code{"color.space"} attribute of the
    image and \code{NULL} is interpreted as the default color space.
    The color space specified here must match the image array
    dimensions, no conversions are performed. Currently the only
    supported non-default color space is \code{"CMYK"} for four-channel
    images (which would be interpreted as \code{"RGBA"} if the color
    space is not specified).}    
}
\value{
  \code{NULL} if the target is either a file or connection, or a raw
  vector containing the compressed JPEG image if the target was a raw
  vector.
}
\details{
  \code{writeJPEG} takes an image as input and compresses it into JPEG
  format. The image input is usually a matrix (for grayscale images -
  dimensions are width, height) or an array (for color and alpha
  images - dimensions are width, height, planes) of reals. The planes
  are interpreted in the sequence red, green, blue, alpha. For
  convenience \code{writeJPEG} allows the source to include alpha
  channel, but JPEG does NOT support alpha channel so it will be
  blended against the specified background.

  Alternative representation of an image is of \code{nativeRaster} class
  which is an integer matrix with each entry representing one pixel in
  binary encoded RGBA format (as used internally by R). It can be
  obtained from \code{\link{readJPEG}} using \code{native = TRUE}.

  Finally, \code{writeJPEG} also supports raw array containing the RGBA
  (or CMYK) image as bytes. The dimensions of the raw array have to be
  planes, width, height (because the storage is interleaved). Currently
  only 4 planes (RGBA and CMYK) are supported and the processing of RGBA
  is equivalent to that of a native raster.

  The result is either stored in a file (if \code{target} is a file
  name), send to a binary connection (if \code{target} is a connection)
  or stored in a raw vector (if \code{target} is a raw vector).
}
%\references{
%}
%\author{
%}
\note{
  Currently \code{writeJPEG} only produces 8-bit, non-progressive JPEG
  format with no additional tags.
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
# it will be slightly different since JPEG is a lossy format
# but not more than a few percent at most
max(abs(img - img2))
# try to write a native raster
img3 <- readJPEG(system.file("img","Rlogo.jpg",package="jpeg"), TRUE)
r2 <- writeJPEG(img3, raw())
img4 <- readJPEG(r2, TRUE)
max(abs(img3 - img4))
}
\keyword{IO}
