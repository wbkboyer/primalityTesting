#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

struct FactoredNumber{
    int num;
    int r;
    int d;
};

/////
// CONFIG
void confirmNumPrimesToGenerate(int numArgs, char **argsToMain, int *numPrimes, int *startingAfter, int *accuracyFactor);
bool getNumPrimesToGenerate(int *numPrimes, int *startingAfter, int *accuracyFactor);

// PRIMALITY TESTING
int* generatePrimes(int numPrimes, int startingAfter, int accuracyFactor);
int generateNextPrime(int currentPrime, int accuracyFactor);
bool millerRabin(int primeCandidate, int accuracyFactor);
bool probabilisticMRCore(int primeCandidate, struct FactoredNumber *factored, int accuracyFactor);
bool deterministicMRCore(int primeCandidate, struct FactoredNumber *factored);

// HELPER FUNC
void factorOutPowersOf2(int numToFactor, struct FactoredNumber *factored);
int randNumInRange(int minVal, int maxVal);
int* randomSequence(int minVal, int maxVal, int sequenceLength);
int* generatePool(int minVal, int maxVal);
int modularExponentiation(int base, int power, int modulus);
int modularMultiplication(int firstTerm, int secondTerm, int modulus);
int integerPower(int base, int power);

/////

int main(int argc, char **argv) {
    static int numPrimes;
    static int accuracyFactor;
    static int startingAfter;
    static bool useDeterministic;

    if (argc == 1) {
        // need to get num primes to generate from stdin
        useDeterministic = getNumPrimesToGenerate(&numPrimes, &startingAfter, &accuracyFactor);
    }
    else if ((argc == 2) || (argc == 3) || (argc == 4)) {
        int numArgs = argc - 1;
        char **argsToMain = (argv + 1);
        confirmNumPrimesToGenerate(numArgs, argsToMain, &numPrimes, &startingAfter, &accuracyFactor);
    }
    else {
        printf("Only enter one integer indicating how many primes to generate when passing in args.\n");
        useDeterministic = getNumPrimesToGenerate(&numPrimes, &startingAfter, &accuracyFactor);
    }
    if (useDeterministic == true) {
        accuracyFactor = -1; // will signal to use deterministic MR algo
    }
    int *primes = generatePrimes(numPrimes, startingAfter, accuracyFactor);
    int *currentPrime = primes;
    printf("Here are the %d primes you requested, starting after %d:\n", numPrimes, startingAfter);
    for (int i = 0; i < numPrimes; i++) {
        printf("%d\n", *(currentPrime++));
    }
    free(primes);
}

void confirmNumPrimesToGenerate(int numArgs, char **argsToMain, int *numPrimes, int *startingAfter, int *accuracyFactor) {
    char correctNumPrimes;
    bool invalidChoice = true;

    *numPrimes = (int)strtol(*(argsToMain), NULL, 10);
    *numPrimes = (*numPrimes > 0) ? *numPrimes : 1; // at least one prime
    *accuracyFactor = (numArgs >= 2) ? (int)strtol(*(argsToMain+1), NULL, 10) : 10; // default: use probabilistic algorithm
    *startingAfter = (numArgs == 3) ? (int)strtol(*(argsToMain+2), NULL, 10) : 2; // default: all primes must be from 2 onwards
    do {
        printf("You requested %d primes to be generated, starting from %d\nwith an accuracy of %d; is this correct? ", *numPrimes, *startingAfter, *accuracyFactor);
        if (*accuracyFactor <= 10) {
            printf("As well, since\nyou chose an accuracy factor of %d, it is recommended\nthat either you continue and use the deterministic\nMiller Rabin algorithm, or that you choose\na larger value for your accuracy factor; include this fact\nin considering your answer.\n", *accuracyFactor);
        }
        printf("\n\tType Y for yes and N for no.\n");
        scanf("%c", &correctNumPrimes);
        if (correctNumPrimes == 'Y' || correctNumPrimes == 'y') {
            break;
        }
        else if (correctNumPrimes == 'N' || correctNumPrimes == 'n') {
            getNumPrimesToGenerate(numPrimes, startingAfter, accuracyFactor);      
        }
    } while (invalidChoice);
}

bool getNumPrimesToGenerate(int *numPrimes, int *startingAfter, int *accuracyFactor) {
    char useDeterministic;
    do {
        printf("How many primes shall be generated?\n");
        scanf("%d", numPrimes);
        
        printf("From where shall we start listing primes? (defaults to all primes from 2 onwards)\n");
        scanf("%d", startingAfter);

        printf("Roughly how accurate do you wish this probabilistic approach to be? \n\t N.B. Enter some number greater than 10, so as to reduce\n\tthe chance of falsely identifying a composite number as the next possible prime.");
        scanf("%d", accuracyFactor);

        if (*accuracyFactor < 10) {
            printf("You chose an accuracy factor less than 10; do you wish to use the deterministic Miller Rabin algorithm instead? Y or N:");
            scanf("%c", &useDeterministic);

            if (useDeterministic == 'Y' || useDeterministic == 'y') {
                return true; // use deterministic Miller Rabin algorithm
            }
            else {
                return false; // Use probabilitic Miller rabin with small accuracy factor
            }
        } 
    } while ((*numPrimes < 1) || (*accuracyFactor < 10));
    return false; // use probabilistic Miller Rabin
} 

int* generatePrimes(int numPrimes, int startingAfter, int accuracyFactor) {
    int *primes;
    primes = (int *)malloc(numPrimes * sizeof(int));
    int *currentPrime = primes; // pointer to current index of array of ints
    int *previousPrime;
    // find if startingAfter is prime, or what the first prime following startingAfter is
    *currentPrime = generateNextPrime(startingAfter - 1, accuracyFactor); // start list with first prime
    primes[0] = generateNextPrime(startingAfter - 1, accuracyFactor);
    for (int i = 1; i < numPrimes; i++) {
        previousPrime = currentPrime++; // assign pointer currentPrime to previousPrime, then increment currentPrime's pointer
        *currentPrime = generateNextPrime(*previousPrime, accuracyFactor); // pass value at current pointer index
    } 
    return(primes);    
}

int generateNextPrime(int currentPrime, int accuracyFactor) {
    int primeCandidate = currentPrime;
    bool isNotPrime = true;
    do {
        primeCandidate++;
        if (!(primeCandidate % 2) || !(primeCandidate % 3) || !(primeCandidate % 5) || !(primeCandidate % 7)) {
            // Get rid of these suckers right off the bat
            continue;
        }
        else {
            isNotPrime = millerRabin(primeCandidate, accuracyFactor);
        }
    } while (isNotPrime);
    return primeCandidate;
}

bool millerRabin(int primeCandidate, int accuracyFactor) {
    struct FactoredNumber *oneLess = (struct FactoredNumber *)malloc(sizeof(struct FactoredNumber));
    if (oneLess == NULL) {
        printf("Couldn't allocate memory!");
        exit(1);
    }

    factorOutPowersOf2(primeCandidate - 1, oneLess);
    if (accuracyFactor > 0) {
        return probabilisticMRCore(primeCandidate, oneLess, accuracyFactor);
    }
    else {
        return deterministicMRCore(primeCandidate, oneLess);    
    }
}

bool probabilisticMRCore(int primeCandidate, struct FactoredNumber *oneLess, int accuracyFactor) {
    int randNum, x;
    // http://www.cplusplus.com/forum/beginner/5131/
    // Don't want to call srand more than once
    time_t t;
    srand((unsigned)time(&t));
    int *randSeq = randomSequence(2, primeCandidate - 2, accuracyFactor);

    for (int i = 0; i < accuracyFactor; i++) {
        if (accuracyFactor > (primeCandidate - 4)) {
            break;
        }
        randNum = *(randSeq + i);
        x = modularExponentiation(randNum, oneLess->d, primeCandidate);

        if ((x == 1) || (x == oneLess->num)) {
            continue;
        }
        for (int j = 0; j < oneLess->r; j++) {
            x = modularExponentiation(x, 2, primeCandidate);
            if (x == 1) {
                return true; // is composite
            }
            else if (x == oneLess->num) {
                break;
            }
        } 
        return true; // is composite
    }
    return false; // is probably prime
}

bool deterministicMRCore(int primeCandidate, struct FactoredNumber *oneLess) {
    int x, y;
    int maxIter = fmin(oneLess->num, floor(2*pow(log(primeCandidate), 2)));
    for (int i = 2; i <= maxIter; i++) {
        x = modularExponentiation(i, oneLess->d, primeCandidate);
        if (x == 1) {
            continue;
        }
        for (int j = 0; j < oneLess->r; j++) {
            y = modularExponentiation(i, integerPower(2, j) * oneLess->d, primeCandidate);
            if (y == oneLess->num) {
                return false; // is prime
            }
        } 
    }
    return true; // is composite 
}

/// HELPER FUNCTIONS
int modularExponentiation(int base, int power, int modulus) {
    int result = 1;
    for (int i = 0; i < power; i++) {
        result *= base;
        result %= modulus;
    }
    return result;
}

int modularMultiplication(int firstTerm, int secondTerm, int modulus) {
    // Recall: ((a*b) mod n) == (((a mod n)*(b mod n)) mod n)
    return(((firstTerm % modulus) * (secondTerm % modulus)) % modulus); 
}

int integerPower(int base, int power) {
    int result = 1;
    for (int i = 0; i < power; i++) {
        result *= base;
    }
    return result;
}

void factorOutPowersOf2(int numToFactor, struct FactoredNumber *factored) {
    int powerOf2 = 0;
    factored->num = numToFactor;
    int d = numToFactor;

    while (d > 1 && (d % 2) == 0) {
        powerOf2++;
        // https://stackoverflow.com/a/6357114/5456879
        // Don't use bitwise shift due to obscurity
        // d = d >> 1; // bit shift to the right <==> divide by 2
        d /= 2;
    }

    factored->r = powerOf2;
    factored->d = d;
}

int* randomSequence(int minVal, int maxVal, int sequenceLength) {
    // https://stackoverflow.com/a/5064432/5456879
    // Create total pool of numbers to draw from
    int poolSize = maxVal - minVal;
    int *pool = generatePool(minVal, maxVal);

    int *randomSeq = (int *)malloc(sequenceLength * sizeof(int));
    int *currentElem = randomSeq;
    int randomIndex;
    for (int i = 1; i <= sequenceLength; i++) {
        if (poolSize < 1) {
            break;
        }
        randomIndex = randNumInRange(0, poolSize); // choose random index of a pool element
        *(currentElem++) = *(pool + randomIndex); // add that element to our sequence
        *(pool + randomIndex) = *(pool + poolSize); // replace that element in the pool with the last element of the pool
        poolSize--; // Decrease pool size to ignore that last element
    }
    printf("\n");
    return randomSeq;
}

int* generatePool(int minVal, int maxVal) {
    int poolSize = maxVal - minVal;
    int *pool = (int *)malloc(poolSize * sizeof(int));
    int *currentPoolMember = pool;
    for (int i = 0; i <= poolSize; i++) {
        *(currentPoolMember++) = minVal + i;
    }
    return pool;
}

int randNumInRange(int minVal, int maxVal) {
    return (rand() % (maxVal - minVal)) + minVal;
}
