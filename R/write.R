writeJPEG <- function(image, target, quality = 0.7, bg = "white")
  invisible(.Call("write_jpeg", image, if (is.raw(target)) target else path.expand(target), quality, bg, PACKAGE="jpeg"))
