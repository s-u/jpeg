writeJPEG <- function(image, target = raw(), quality = 0.7, bg = "white") {
  if (inherits(target, "connection")) {
    r <- .Call("write_jpeg", image, raw(), quality, bg, PACKAGE="jpeg")
    writeBin(r, target)
    invisible(NULL)
  } else invisible(.Call("write_jpeg", image, if (is.raw(target)) target else path.expand(target), quality, bg, PACKAGE="jpeg"))
}
