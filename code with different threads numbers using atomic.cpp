#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <chrono>
#include <omp.h>

namespace fs = std::filesystem;

// Recursive function to check if a file is empty
bool isFileEmpty(const std::string& filepath) {
    std::ifstream file(filepath);
    return file.peek() == std::ifstream::traits_type::eof(); 
}

// Function to search for a keyword within a file
bool searchInFile(const std::string& filepath, const std::string& keyword) {
    
    if (isFileEmpty(filepath)) {
        std::cout << "File is empty: " << filepath << std::endl;
        return false;
    }

    std::ifstream file(filepath);
    std::string line;
	
    size_t lineNumber = 0;
    bool found = false;
	
    // Inner loop: Iterate through each line in the file
    while (std::getline(file, line)) {
		#pragma omp atomic 
		// race condition 
        ++lineNumber; 
        
        // Search for the keyword in the current line
        if (line.find(keyword) != std::string::npos) {
            std::cout << "Keyword found in file: " << filepath 
                      << " at line: " << lineNumber << std::endl;
		// #pragma omp 
			// race condition
            found = true;
		
        }
    }
    return found;
}

int main() {
	
	int thread_count;
    std::cout << "Enter the number of threads (1, 2, 4, 8): ";
    std::cin >> thread_count;
    omp_set_num_threads(thread_count);
    
    std::string directory;
    std::cout << "Enter the directory to search in: ";
    std::cin >> directory;

    
    std::string keyword;
    std::cout << "Enter the keyword to search: ";
    std::cin >> keyword;

    // Track search duration
    auto start = std::chrono::high_resolution_clock::now();
    size_t filesWithKeyword = 0;
	
	
    // Outer loop: Iterate over each file in the directory
	#pragma omp parallel shared(filesWithKeyword)
{
    #pragma omp single
    {
        for (const auto& entry : fs::directory_iterator(directory)) {
            if (fs::is_regular_file(entry)) {
                const std::string filepath = entry.path().string();
                #pragma omp task shared(filesWithKeyword)
                {
                    if (searchInFile(filepath, keyword)) {
						#pragma omp atomic 
						
							// race condition 
                            ++filesWithKeyword;
                        
                    }
                }
            }
        }
    }
}
    // Calculate and display search duration
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    std::cout << "Total files with keyword: " << filesWithKeyword << std::endl;
    std::cout << "Total time taken: " << duration.count() << " seconds" << std::endl;

    return 0;
}

