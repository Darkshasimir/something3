///////////////////////////////////////////////////////////////////////////////
// maxprotein.hh
//
// Compute the set of foods that maximizes protein, within a calorie budget,
// with the greedy method or exhaustive search.
//
///////////////////////////////////////////////////////////////////////////////


#pragma once

#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <queue>
#include <sstream>
#include <string>
#include <vector>

// One food item in the USDA database.
class Food {
private:
  // Human-readable description of the food, e.g. "all-purpose wheat
  // flour". Must be non-empty.
  std::string _description;

  // Human-readable description of the amount of the food in one
  // sample, e.g. "1 cup". Must be non-empty.
  std::string _amount;

  // Number of grams in one sample; must be non-negative.
  int _amount_g;

  // Energy, in units of kilocalories (commonly called "calories"), in
  // one sample; must be non-negative.
  int _kcal;

  // Number of grams of protein in one sample; most be non-negative.
  int _protein_g;

public:
  Food(const std::string& description,
       const std::string& amount,
       int amount_g,
       int kcal,
       int protein_g)
    : _description(description),
      _amount(amount),
      _amount_g(amount_g),
      _kcal(kcal),
      _protein_g(protein_g) {

    assert(!description.empty());
    assert(!amount.empty());
    assert(amount_g >= 0);
    assert(kcal >= 0);
    assert(protein_g >= 0);
  }

  const std::string& description() const { return _description; }
  const std::string& amount() const { return _amount; }
  int amount_g() const { return _amount_g; }
  int kcal() const { return _kcal; }
  int protein_g() const { return _protein_g; }

};

// Alias for a vector of shared pointers to Food objects.
typedef std::vector<std::shared_ptr<Food>> FoodVector;

// Load all the valid foods from a USDA database in their ABBREV
// format. Foods that are missing fields such as the amount string are
// skipped. Returns nullptr on I/O error.
std::unique_ptr<FoodVector> load_usda_abbrev(const std::string& path) {

  std::unique_ptr<FoodVector> failure(nullptr);
  
  std::ifstream f(path);
  if (!f) {
    return failure;
  }

  std::unique_ptr<FoodVector> result(new FoodVector);

  for (std::string line; std::getline(f, line); ) {

    std::vector<std::string> fields;
    std::stringstream ss(line);
    for (std::string field; std::getline(ss, field, '^'); ) {
      fields.push_back(field);
    }

    if (fields.size() != 53) {
      return failure;
    }
    
    std::string descr_field = fields[1],
                kcal_field = fields[3],
                protein_g_field = fields[4],
                amount_g_field = fields[48],
                amount_field = fields[49];

    auto remove_tildes = [](std::string& output,
			    const std::string& field) {
      if ((field.size() < 3) ||
	  (field.front() != '~') ||
	  (field.back() != '~')) {
	return false;
      } else {
	output.assign(field.begin() + 1, field.end() - 1);
	return true;
      }
    };

    auto parse_mil = [](int& output, const std::string& field) {
      std::stringstream ss(field);
      double floating;
      ss >> floating;
      if ( ! ss ) {
	return false;
      } else {
	output = lround(floating);
	return true;
      }
    };

    std::string description, amount;
    int amount_g, kcal, protein_g;
    if ( remove_tildes(description, descr_field) &&
	 remove_tildes(amount, amount_field) &&
	 parse_mil(amount_g, amount_g_field) &&
	 parse_mil(kcal, kcal_field) &&
	 parse_mil(protein_g, protein_g_field) ) {
      result->push_back(std::shared_ptr<Food>(new Food(description,
						       amount,
						       amount_g,
						       kcal,
						       protein_g)));
    }
  }

  f.close();

  return result;
}

// Convenience function to compute the total kilocalories and protein
// in a FoodVector. Those values are returned through the
// first two pass-by-reference arguments.
void sum_food_vector(int& total_kcal,
		     int& total_protein_g,
		     const FoodVector& foods) {
  total_kcal = total_protein_g = 0;
  for (auto& food : foods) {
    total_kcal += food->kcal();
    total_protein_g += food->protein_g();
  }
}

// Convenience function to print out each food in a FoodVector,
// followed by the total kilocalories and protein in it.
void print_food_vector(const FoodVector& foods) {
  for (auto& food : foods) {
    std::cout << food->description()
	      << " (100 g where each " << food->amount()
	      << " is " << food->amount_g() << " g)"
	      << " kcal=" << food->kcal()
	      << " protein=" << food->protein_g() << " g"
	      << std::endl;
  }
  
  int total_kcal, total_protein_g;
  sum_food_vector(total_kcal, total_protein_g, foods);
  std::cout << "total kcal=" << total_kcal
	    << " total_protein=" << total_protein_g << " g"
	    << std::endl;
}

// Filter the vector source, i.e. create and return a new FoodVector
// containing the subset of the foods in source that match given
// criteria. This is intended to 1) filter out foods with zero
// calories that are irrelevant to our optimization, and 2) limit the
// size of inputs to the exhaustive search algorithm since it will
// probably be slow. Each food that is included must have at least
// min_kcal kilocalories and at most max_kcal kilocalories. In
// addition, the the vector includes only the first total_size foods
// that match these criteria.
std::unique_ptr<FoodVector> filter_food_vector(const FoodVector& source,
					       int min_kcal,
					       int max_kcal,
					       int total_size) {
	std::unique_ptr<FoodVector> filteredFood(new FoodVector);
	int size = source.size();
	int total_size_counter = 0;
	for(int n = 0; n < size && total_size_counter < total_size; n++)
	{
		if(source[n]->kcal() !=0 && source[n]->kcal() >= min_kcal && source[n]->kcal() <= max_kcal)
		{
			filteredFood->push_back(source[n]);
			total_size_counter++;
		}
	}
	return filteredFood;
}

// Compute the optimal set of foods with a greedy
// algorithm. Specifically, among the food items that fit within a
// total_kcal calorie budget, choose the food whose protein is
// greatest. Repeat until no more foods can be chosen, either because
// we've run out of foods, or run out of calories.
std::unique_ptr<FoodVector> greedy_max_protein(const FoodVector& foods,
					       int total_kcal) {
	FoodVector todo(foods);
	std::unique_ptr<FoodVector> result(new FoodVector);
	int size = todo.size();
	int protein = 0;
	int result_cal = 0;
	int protein_location;
	int c;
	while (size != 0)
	{
		for(int i = 0; i < size; i++)
		{
			if(protein < todo[i]->protein_g())
			{
				protein = todo[i]->protein_g();
				protein_location = i;
				c = todo[i]->kcal();
			}
		}
		if (result_cal + c <= total_kcal)
		{
			result->push_back(todo[protein_location]);
			result_cal += c;
		}
		todo.erase(todo.begin()+protein_location);
		size -= 1;
		protein = 0;
	}
	return result;
}

// Compute the optimal set of foods with an exhaustive search
// algorithm. Specifically, among all subsets of foods, return the
// subset whose calories fit within the total_kcal budget, and whose
// total protein is greatest. To avoid overflow, the size of the foods
// vector must be less than 64.
std::unique_ptr<FoodVector> exhaustive_max_protein(const FoodVector& foods,
						   int total_kcal) {
 	const int n = foods.size();
	int total_calories= 0;
	int total_protein= 0;
	int total_protein_best = 0;
	int size;
	FoodVector candidate(foods);
	candidate.clear();
	assert(n < 64);
	std::unique_ptr<FoodVector> result(new FoodVector);
	for (uint64_t bits = 0; bits < pow(2.0,n); bits++)
	{
		for (int j = 0; j < n; j++)
		{
			if (((bits >> j) & 1) == 1)
				candidate.push_back(foods[j]);
		}
		//sum_food_vector(total_calories, total_protein, &candidate);
		size = candidate.size();
		std::cout << "\n";
		std::cout << "Size :" << size << "\n";
		for (int i = 0; i < size; i++) 
		{
			total_calories += candidate[i]->kcal();
			total_protein += candidate[i]->protein_g();
			std::cout << "food :" << candidate[i]->description() << "\n";
			std::cout << "protein :" << candidate[i]->protein_g() << "\n";
			std::cout << "kcal :" << candidate[i]->kcal() << "\n";
		}
		std::cout << "food :" << candidate << "\n";
		std::cout << "total :" << total_calories << "\n";
		std::cout << "max :" << total_kcal << "\n";
		if(total_calories <= total_kcal)
		{
			if(total_protein_best == 0 || total_protein > total_protein_best)
			{
				result->clear();
				for (int i = 0; i < size; i++)
					result->push_back(candidate[i]);
				total_protein_best = total_protein;
			}
		}
		candidate.clear();
	}
	return result;
}
