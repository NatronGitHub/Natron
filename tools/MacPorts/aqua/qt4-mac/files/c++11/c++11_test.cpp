// from Qt 5.0

#if __cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X__)
#else
#  error "__cplusplus must be >= 201103L, or __GXX_EXPERIMENTAL_CXX0X__ must be defined"
#endif

constexpr int get_five() {return 5;}

int some_value[get_five() + 7];

int main(int, char **) { return 0; }
