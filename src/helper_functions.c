int pow10(int order) {
  // Returns an integer 10^order
  int value = 1;
  for (int i = 0; i < order; ++i) {
    value *= 10;
  }
  return value;
}

