led8x8 <-
function(txt) {
  out=.C("led8x8", as.character(txt))
}
