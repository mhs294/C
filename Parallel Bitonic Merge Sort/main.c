/****************************************************	
*	Maximilian Schroder                             *
*													*
*	This program will sort a list of numbers read   *
*   from a file (inFile), with the amount of       *
*   numbers to be sorted (total) and starting file  *
*   index (start) specified by the user. The sorted *
*   list of numbers will be written to a file       *
*   (outFile) at the end of the program.            *
*                                                   *
*	To compile and run:								*
*	mpicc main.c -o hw2                             *
*	hw2 <total> <inFile> <outFile>                  *
*   (NOTE: outFile is optional)                     *
****************************************************/

#define LOW 0
#define HIGH 1

#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include <time.h>

// sends/receives broadcast from master and closes program if error flag buffer is set
void check_error(int * error)
{
    // receive error flag broadcast from master process
    MPI_Bcast(error, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    // if error flag is set, close program
    if (error[0])
    {
        // call MPI_Finalize
        MPI_Finalize();
        
        // exit with error status
        exit(error[0]);
    }
}

// sorts the specified array using radix sort algorithm
void local_sort(int * nums, int total)
{
    // radix sort variables
    int i;                    // for loop iterator
    int * radix;              // radix array
    int max = 0;              // maximum value in nums array
    int exp = 1;              // exponent value
    
    // initialize radix array and set contents to 0
    radix = (int *)malloc(total * sizeof(int));
    
    // find maximum value in nums array
    for (i = 0; i < total; i++)
    {
        if (nums[i] > max)
            max = nums[i];
    }
    
    // loop until current radix exceeds max
    while (max / exp > 0)
    {
        // create a bucket array for digits (0-9) and initialize its contents to 0
        int bucket[10] = { 0 };
        
        // store count of digits of all elements in nums for current radix
        for (i = 0; i < total; i++)
        {
            bucket[(nums[i] / exp) % 10]++;
        }
        
        // add counts of all buckets prior to current bucket to current bucket's count
        for (i = 1; i < 10; i++)
        {
            bucket[i] += bucket[i - 1];
        }
        
        // going from end to start, insert elements from nums array into radix array sorted by current radix's digit 
        for (i = total - 1; i >= 0; i--)
        {
            radix[--bucket[(nums[i] / exp) % 10]] = nums[i];
        }
        
        // copy all elements from radix array into nums array
        for (i = 0; i < total; i++)
        {
            nums[i] = radix[i];
        }
        
        // increase exponent to update current radix
        exp *= 10;
    }
    
    // free memory allocated to radix
    free(radix);
}

// swaps the contents of the specified array using iterative bitonic sort algorithm
void bitonic_swap(int start, int gap, int spread, int * nums, int mode)
{
    // bitonic swap variables
    int i, j;                                           // for loop iterators
    int * newLeft = (int *)malloc(gap * sizeof(int));   // array containing elements in new left swap section
    int * newRight = (int *)malloc(gap * sizeof(int));  // array containing elements in new right swap section
    int nl = 0;                                         // current index of new left swap section
    int nr = 0;                                         // current index of new right swap section
    
    // iterate through elements in the specified swap sections of nums array
    for (i = start, j = start + gap; i < start + spread && j < start + gap + spread; i++, j++)
    {        
        // if performing a low swap, insert the lowest elements into newLeft array
        if (mode == LOW)
        {
            // if newLeft array has not yet been filled, insert current element into newLeft array
            if (nl < spread)
            {
                if (nums[i] > nums[j])
                {
                    // add current right swap element to newLeft array
                    newLeft[nl] = nums[j];
                    
                    // shift left swap section pointer backward
                    i--;
                }
                else
                {
                    // add current left swap element to newLeft array
                    newLeft[nl] = nums[i];
                    
                    // shift right swap section pointer backward
                    j--;
                }
                
                // increment new left swap section index
                nl++;
            }
            else
            {
                if (nums[i] > nums[j])
                {
                    // add current right swap element to newRight array
                    newRight[nr] = nums[j];
                    
                    // shift left swap section pointer backward
                    i--;
                }
                else
                {
                    // add current left swap element to newRight array
                    newRight[nr] = nums[i];
                    
                    // shift right swap section pointer backward
                    j--;
                }
                
                // increment new left swap section index
                nr++;
            }
        }
        else if (mode == HIGH)
        {
            // if newRight array has not yet been filled, insert current element into newRight array
            if (nr < spread)
            {
                if (nums[i] > nums[j])
                {
                    // add current right swap element to newRight array
                    newRight[nr] = nums[j];
                    
                    // shift left swap section pointer backward
                    i--;
                }
                else
                {
                    // add current left swap element to newRight array
                    newRight[nr] = nums[i];
                    
                    // shift right swap section pointer backward
                    j--;
                }
                
                // increment new left swap section index
                nr++;
            }
            else
            {
                if (nums[i] > nums[j])
                {
                    // add current right swap element to newLeft array
                    newLeft[nl] = nums[j];
                    
                    // shift left swap section pointer backward
                    i--;
                }
                else
                {
                    // add current left swap element to newLeft array
                    newLeft[nl] = nums[i];
                    
                    // shift right swap section pointer backward
                    j--;
                }
                
                // increment new left swap section index
                nl++;
            }
        }
    }
    
    // insert remaining swap section elements into new right/left swap section depending on mode
    if (i < start + spread)
    {
        for (; i < start + spread; i++, nl++, nr++)
        {
            if (mode == LOW)
            {
                newRight[nr] = nums[i];
            }
            else if (mode == HIGH)
            {
                newLeft[nl] = nums[i];
            }
        }
    }
    else if (j < start + gap + spread)
    {
        for (; j < start + gap + spread; j++, nl++, nr++)
        {
            if (mode == LOW)
            {
                newRight[nr] = nums[j];
            }
            else if (mode == HIGH)
            {
                newLeft[nl] = nums[j];
            }
        }
    }
    
    // set current left and right swap sections to new left and new right swap sections
    for (i = start, j = start + gap, nl = 0, nr = 0; nl < spread && nr < spread; i++, j++, nl++, nr++)
    {        
        nums[i] = newLeft[nl];
        nums[j] = newRight[nr];
    }
    
    // free memory allocated to newLeft and newRight
    free(newLeft);
    free(newRight);
}


// main routine
int main(int argc, char ** argv)
{	
	// global variables
    int i;                                      // for loop iterator
    int * error = (int *)malloc(sizeof(int));   // error flag buffer
	int progid;                                 // this program's id (rank)
    int numprocs;                               // number of processors used for computation
    int * myTotal = (int *)malloc(sizeof(int)); // amount of numbers to be stored in myNums
    int * myNums;                               // array storing this processor's portion of the list
    int myNumsTotal;                            // amount of numbers to be stored in myNums
    
    // master only variables
    double startwtime;                          // variable for start timestamp
    double endwtime;                            // variable for end timestamp
    double totalwtime = 0.0;                    // total time elapsed
    int total;                                  // amount of numbers to be sorted
    int * allNums;                              // array storing all numbers in list (used by root only)
    int dummy = 0;                              // smallest number that does not exist in number list
    int dummyNums = 0;                          // amount of dummy numbers to be added to number list
    FILE * inFile;                              // input file pointer
    FILE * outFile;                             // output file pointer
	
	// initialize MPI with args
    MPI_Init(&argc, &argv);
	
	// define this program's rank
	MPI_Comm_rank(MPI_COMM_WORLD, &progid);
    
    // define number of processors for MPI
	MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
	
	// (master only) variable and file stream initialization
    if (progid == 0)
    {
        //////////////////////////////
        //                          //
        //  ERROR CHECKING          //
        //                          //
        //////////////////////////////
        
        // initialize error flag buffer
        error[0] = 0;
        
        // initialize total
        total = atoi(argv[1]);
        
        // if input file is specified, initialize input file stream
        if (argc > 2)
        {
            inFile = fopen(argv[2], "r");
        }
        
        // check for valid total
        if (total < 1)
        {
            // invalid total specified
            fprintf(stderr, "Invalid total specified (%s). Please enter a positive, nonzero total.\n", argv[1]);
            
            // set error flag buffer
            error[0] = 1;
            
            // call check_error to close program
            check_error(error);
        }
        
        // if input file is specified, check for successful input file stream initialization
        if (argc > 2 && !inFile)
        {
            // inFile initialize failed
            fprintf(stderr, "Failed to initialize input file stream (%s).\n", argv[2]);
            
            // set error flag buffer
            error[0] = 1;
            
            // call check_error to close program
            check_error(error);
        }
		
        //////////////////////////////
        //                          //
        //  ARRAY INITIALIZATION    //
        //                          //
        //////////////////////////////        
        
        // temporary variables
        int realTotal = 0;          // actual amount of numbers read from input file
        int offset = 0;             // offset for starting index
        int currentNum = 0;         // current number read from input file
        int currentIndex = 0;       // current index of the allNums array
        int power = 2;              // current binary power
        
        // if input file is specified, check validity of specified total
        if (argc > 2)
        {
            // verify contents of input file against specified total
            while (fscanf(inFile, "%d", &currentNum) != EOF)
            {
                realTotal++;
            }
            
            // update total if total is greater than amount of available numbers
            if (total > realTotal)
            {
                // print message notifying user of discrepancy
                fprintf(stdout, "Specified total (%d) > available numbers (%d).\n",
                        total, realTotal);
                fprintf(stdout, "New total = %d.\n", realTotal);
                
                // update total
                total = realTotal;
            }
        }
		
        // calculate number of dummy numbers needed to reach bitonic total (total = 2^n, n > 0)
        while (power < total)
        {
            if (total % power != 0)
			{
				// update dummyNums count
                dummyNums += (total % power);
				
				// update total to include added dummyNums
				total += (total % power);
			}
            power *= 2;
        }
        
        // allocate memory for allNums array
        allNums = (int *)malloc(total * sizeof(int));        
        
        // insert dummy numbers at front of array
        for (currentIndex = 0; currentIndex < dummyNums; currentIndex++)
        {
            allNums[currentIndex] = dummy;
        }
        
        // if input file is specified, insert numbers from file into allNums array; else generate randomly
        if (argc > 2)
        {
            // reset input file pointer
            rewind(inFile);
            
            // read numbers from input file and store them in allNums array
            while (fscanf(inFile, "%d", &currentNum) != EOF && currentIndex < total)
            {            
                allNums[currentIndex] = currentNum;
                currentIndex++;
            }
        }
        else
        {
            // seed time for random number generation
            srand(time(NULL));
            
            // generate random numbers between 1 and 1000000000
            for (; currentIndex < total; currentIndex++)
            {
                allNums[currentIndex] = (rand() % 1000000000) + 1;
            }
        }
        
        // check to make sure we've inserted the correct amount of numbers into the allNums array
        if (currentIndex > total)
        {
            // allNums array population failed
            fprintf(stderr, "Incorrect number of items inserted into allNums (%d, total = %d).\n", currentIndex, total);
            
            // set error flag buffer
            error[0] = 1;
            
            // call check_error to close program
            check_error(error);
        }
        
        // calculate myTotal
        myTotal[0] = total / numprocs;
        
        // if input file is specified, close input file stream
        if (argc > 2)
        {
            fclose(inFile);
        }
        
        // call check error to indicate successful initialization to slave processes
        check_error(error);
    }
    
    // (slave only) check for initialization error
    if (progid != 0)
    {
        // call check_error to see if program needs to close
        check_error(error);
    }
    
    // broadcast myTotal from master to slave process
    MPI_Bcast(myTotal, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    // allocate memory for myNums
    myNums = (int *)malloc(myTotal[0] * sizeof(int));
    
    //////////////////////////////
    //                          //
    //  BITONIC SORT ROUTINE    //
    //                          //
    //////////////////////////////
    
    // global bitonic sort variables
    int gap;                // current displacement between swap sections in the allNums array
    
    // master only bitonic sort variables
    int * lowhigh;          // array containing swap instructions for each bitonic iteration
    int swapGap;            // gap length of current bitonic swap iteration
    
    // (master only) initialize lowhigh if we are running in parallel
    if (progid == 0 && numprocs > 1)
    {
        lowhigh = (int *)malloc((numprocs / 2) * sizeof(int));
        for (i = 0; i < numprocs / 2; i++)
        {
            if (i % 2 == 0)
            {
                lowhigh[i] = LOW;
            }
            else if (i % 2 != 0)
            {
                lowhigh[i] = HIGH;
            }
        }
    }
    
    // iterative bitonic swapping routine
    for (gap = total / (numprocs * 2); gap <= total / 2; gap *= 2)
    {	        	
        // (master only) perform bitonic swapping on allNums array
        if (progid == 0 && gap >= (total / numprocs))
        {
			// store start timestamp
            startwtime = MPI_Wtime();
		
            // initialize swapGap
            swapGap = gap;
            
            // execute bitonic swap until all bitonic swaps required for current gap have been completed
            while (swapGap >= (total / numprocs))
            {			
                // iterate through elements in allNums array, moving ahead by swapGap * 2 for each iteration
                for (i = 0; i < total; i += myTotal[0])
                {
                    // shift loop iterator to next swap sections of allNums array
                    if (i / swapGap > 0 && i % swapGap == 0)
                    {
                        i += swapGap;
                        
                        // if loop iterator is greater than or equal to total, break loop
                        if (i >= total)
                        {
                            break;
                        }
                    }
                    
                    // perform bitonic swap on current swap sections in allNums array
                    bitonic_swap(i, swapGap, myTotal[0], allNums, lowhigh[i / (2 * gap)]);
                }
                
                // update swapGap for next bitonic swap iteration
                swapGap /= 2;
            }
			
			// store start timestamp
            endwtime = MPI_Wtime();
            
            // update totalwtime
            totalwtime += endwtime - startwtime;
        }
        
        // scatter parts of allNums array to each process
        MPI_Scatter(allNums, myTotal[0], MPI_INT, myNums, myTotal[0], MPI_INT, 0, MPI_COMM_WORLD);
        
        // (master only) start timer for performance data
        if (progid == 0)
        {
            // store start timestamp
            startwtime = MPI_Wtime();
        }
		
        // sort this process's myNums array
        local_sort(myNums, myTotal[0]);
        
        // (master only) stop timer for performance data and update totalwtime
        if (progid == 0)
        {
            // store start timestamp
            endwtime = MPI_Wtime();
            
            // update totalwtime
            totalwtime += endwtime - startwtime;
        }
        
        // gather myNums arrays into allNums array
        MPI_Gather(myNums, myTotal[0], MPI_INT, allNums, myTotal[0], MPI_INT, 0, MPI_COMM_WORLD);
    }
    
    // (master only) write sorted array to output file and print execution results
    if (progid == 0)
    {
        // if output file is specified, write sorted number list to file
        if (argc > 2)
        {
            //initialize output file stream
            outFile = fopen(argv[3], "w");
            
            // check for successful output file stream initialization
            if (!outFile)
            {
                // outFile initialize failed
                fprintf(stderr, "Failed to initialize output file stream (%s).\n", argv[3]);
                
                // set error flag buffer
                error[0] = 1;
                
                // call check_error to close program
                check_error(error);
            }
            
            // write contents of allNums to output file, excluding dummies
            for (i = dummyNums; i < total; i++)
            {
                fprintf(outFile, "%d\n", allNums[i]);
            }
            
            // close output file stream
            fclose(outFile);
        }
        
        // print execution results to screen
        fprintf(stdout, "Total numbers sorted: %s\n", argv[1]);
        fprintf(stdout, "Total processes run: %d\n", numprocs);
        fprintf(stdout, "Time elapsed: %f4s\n", (totalwtime / 1000.0));
        
        // print first ten sorted numbers starting from indexes 100k and 200k
        fprintf(stdout, "\nFirst 10 sorted numbers, starting at index 100,000:\n\n");
        for (i = 100000 + dummyNums; i < 100010 + dummyNums; i++)
        {
            fprintf(stdout, "%d\n", allNums[i]);
        }
        fprintf(stdout, "\nFirst 10 sorted numbers, starting at index 200,000:\n\n");
        for (i = 200000 + dummyNums; i < 200010 + dummyNums; i++)
        {
            fprintf(stdout, "%d\n", allNums[i]);
        }
        
        // call check error to indicate successful finalization to slave processes
        check_error(error);
    }
    
    // (slave only) check for initialization error
    if (progid != 0)
    {
        // call check_error to see if program needs to close
        check_error(error);
    }
    
    // call Finalize
    MPI_Finalize();
	
    // exit with success code
	exit(0);
}
