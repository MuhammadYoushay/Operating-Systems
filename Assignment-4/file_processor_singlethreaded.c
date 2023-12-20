#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
// Structure to hold the dynamic array information which I have created
typedef struct {
    long long int *array_elements;
    size_t is_used;
    size_t size;
} myArray;
//Global variables and initializations 
//LLONG_MAX and LLONG_MIN are macros that are defined in the limits.h
long long int sum = 0;
long long int min = LLONG_MAX;
long long int max = LLONG_MIN;
// Function to initialize and insert elements into the dynamic array. 
void initializeAndInsert(myArray *arr, FILE *file) {
    const size_t startCap = 1;
    //Dynamic memory allocation
    arr->array_elements = (long long int *)malloc(startCap * sizeof(long long int));
    //Checking if the memory is allocated or not
    if (arr->array_elements == NULL) {
        fprintf(stderr, "Cannot Allocate Memory\n");
        exit(EXIT_FAILURE);
    }
    arr->is_used = 0;
    arr->size = startCap;
    long long int currentElement;
    //Reading the file and inserting the elements into the array as long long int
    while (fscanf(file, "%lld", &currentElement) == 1) {
        if (arr->is_used == arr->size) {
            //Doubling the size of the array if the array is full. Following linked list concept
            size_t newSize = arr->size * 2;
            //Reallocating the memory
            long long int *newArray = (long long int *)realloc(arr->array_elements, newSize * sizeof(long long int));
            if (newArray == NULL) {
                fprintf(stderr, "Cannot Allocate Memory\n");
                exit(EXIT_FAILURE);
            }
            arr->array_elements = newArray;
            arr->size = newSize;
        }
        //Inserting the elements into the array
        arr->array_elements[arr->is_used] = currentElement;
        arr->is_used++;
    }
}
// Function to free the memory occupied by the dynamic array
void freeMyArray(myArray *arr) {
    free(arr->array_elements);
    arr->array_elements = NULL;
    arr->is_used = arr->size = 0;
}
//Main function
int main(int argc, char *argv[]) {
    //Checking if the correct number of command line arguments are provided
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <program_name> <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }
    const char *filename = argv[1];
    //Opening the file and reading
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }
    //Creating and populating the dynamic array from the file
    myArray arrayData;
    //Calling the function I created above
    initializeAndInsert(&arrayData, file);
    fclose(file);

    //Timing the processing time as per the code given in the assignment pdf
    clock_t start_time = clock();
    for (size_t i = 0; i < arrayData.is_used; i++) {
        sum += arrayData.array_elements[i];
        if (arrayData.array_elements[i] < min) {
            min = arrayData.array_elements[i];
        }
        if (arrayData.array_elements[i] > max) {
            max = arrayData.array_elements[i];
        }
    }
    clock_t end_time = clock();
    double elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;

    // Printing the results and processing time
    printf("Sum: %lld\n", sum);
    printf("Minimum: %lld\n", min);
    printf("Maximum: %lld\n", max);
    printf("Time taken - Main - : %f seconds\n", elapsed_time);

    // Freeing the memory occupied by the dynamic array by calling the function we created above
    freeMyArray(&arrayData);
    return EXIT_SUCCESS;
}