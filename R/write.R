writeJPEG <- function(image, target)
  invisible(.Call("write_jpeg", image, if (is.raw(target)) target else path.expand(target), PACKAGE="jpeg"))
