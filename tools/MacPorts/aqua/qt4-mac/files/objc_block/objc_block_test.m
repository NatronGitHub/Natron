double (^multiplyTwoValues)(double, double) =
  ^(double firstValue, double secondValue) {
  return firstValue * secondValue;
};

int main () {
  double result = multiplyTwoValues(2,4);
  (void) result;
  return (0);
}
