// RUN: clang-cc -fsyntax-only -verify -parse-noop -fblocks %s

struct blockStruct {
  int (^a)(float, int);
  int b;
};

int blockTaker (int (^myBlock)(int), int other_input)
{
  return 5 * myBlock (other_input);
}

int main (int argc, char **argv)
{
  int (^blockptr) (int) = ^(int inval) {
    printf ("Inputs: %d, %d.\n", argc, inval);
    return argc * inval;
  };


  argc = 10;
  printf ("I got: %d.\n",
          blockTaker (blockptr, 6));
  return 0;
}

