\name{readJPEG}
\alias{readJPEG}
\title{
Read a bitmap image stored in the JPEG format
}
\description{
Reads an image from a JPEG file/content into a raster array.
}
\usage{
readJPEG(source, native = FALSE)
}
\arguments{
  \item{source}{Either name of the file to read from or a raw vector
  representing the JPEG file content.}
  \item{native}{determines the image representation - if \code{FALSE}
  (the default) then the result is an array, if \code{TRUE} then the
  result is a native raster representation.}
}
%\details{
%}
\value{
If \code{native} is \code{FALSE} then an array of the dimensions height
x width x channels. If there is only one channel the result is a
matrix. The values are reals between 0 and 1. If \code{native} is
\code{TRUE} then an object of the class \code{nativeRaster} is
returned instead. The latter cannot be easily computed on but is the
most efficient way to draw using \code{rasterImage}.

Most common files decompress into RGB (3 channels) or
Grayscale (1 channel). Note that Grayscale images
cannot be directly used in \code{\link{rasterImage}} unless
\code{native} is set to \code{TRUE} because \code{rasterImage} requires
RGB or RGBA format (\code{nativeRaster} is always 8-bit RGBA).

JPEG doesn't support alpha channel, you may want to use PNG instead in
such situations.
}
%\references{
%}
%\author{
%}
\note{
  CMYK JPEG images saved by Adobe Photoshop may have inverted ink values due
  to a bug in Photoshop. Unfortunately this includes some sample CMYK
  images that are floating around, so beware of the source when
  converting the result to other color spaces. \code{readJPEG} will
  preserve values exactly as they are encoded in the file.

  Some images use EXIF to indicate that the image is stored in one orientation,
  but should be viewed in another. \code{readJPEG} will return the image as it
  was stored, so refer to EXIF's "orientation" property for more details (e.g.,
  see the \code{read_exif} function in the \code{exifr} package).
}
\seealso{
\code{\link{rasterImage}}, \code{\link{writeJPEG}}
}
\examples{
# read a sample file (R logo)
img <- readJPEG(system.file("img", "Rlogo.jpg", package="jpeg"))

# read it also in native format
img.n <- readJPEG(system.file("img", "Rlogo.jpg", package="jpeg"), TRUE)

# if your R supports it, we'll plot it
if (exists("rasterImage")) { # can plot only in R 2.11.0 and higher
  plot(1:2, type='n')

  rasterImage(img, 1.2, 1.27, 1.8, 1.73)
  rasterImage(img.n, 1.5, 1.5, 1.9, 1.8)
}
}
\keyword{IO}
