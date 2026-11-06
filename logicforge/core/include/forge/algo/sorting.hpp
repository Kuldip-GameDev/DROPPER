#pragma once
#include <vector>

#include "forge/viz/recorder.hpp"

namespace forge::algo {

// All sorts are in-place and ascending. The Recorder only observes; the
// sorting logic is exactly what you would write without visualisation.
void bubbleSort(std::vector<int>& a, viz::Recorder& rec);
void selectionSort(std::vector<int>& a, viz::Recorder& rec);
void insertionSort(std::vector<int>& a, viz::Recorder& rec);
void mergeSort(std::vector<int>& a, viz::Recorder& rec);
void quickSort(std::vector<int>& a, viz::Recorder& rec);
void heapSort(std::vector<int>& a, viz::Recorder& rec);

}  // namespace forge::algo
