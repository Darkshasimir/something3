
#include "maxprotein.hh"
#include "timer.hh"

using namespace std;

int main() {
  // these values can be changed to your liking
  // but it doesn't matter what exactly they are
  int min_kcal = 0; 
  int max_kcal = 2000;
  int total_kcal = 5000;

  // int n = 5;
  // int n = 10;
  // int n = 15;
  int n = 20;
  //int n = 25;

  auto source = load_usda_abbrev("ABBREV.txt");
  auto foods = filter_food_vector(*source, min_kcal, max_kcal, n);

  Timer timer;
  // make sure to swap exhaustive_max_protein with greedy_max_protein
  auto result = greedy_max_protein(*foods, total_kcal);

  double elapsed = timer.elapsed();

  cout << "greedy_max_protein" << endl;
  cout << "n = " << n << endl;
  cout << "elapsed time=" << elapsed << " seconds" << endl;

  // printing out the sum
  // to see how close the greedy/exhaustive search was to total_kcal
  int sum = 0;
  for (auto &x : *result)
  {
    sum += x->kcal();
  }
  cout << "sum = " << sum << endl;

  return 0;
}
