#include <stdio.h>
#include <argp.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

const char *program_version = "Primality Testing v1.0";
const char *program_author = "Wanda Boyer";
const char *program_email = "<wbkboyer@gmail.com>";

/* Program documentation. */
static char doc[] = "Implementation of the probabilistic and deterministic Miller Rabin algorithms to generate primes.";

/* A description of the arguments we accept. */
static char args_doc[] = "numPrimes startingAfter accuracyFactor outfile";

/* The options we understand. 
 * OPTION_ARG_OPTIONAL causes issues with spacing between option and argument*/
static struct argp_option options[] = {
    {"numPrimes",          'n',             "NUM_PRIMES",                  0,
        "How many primes you wish to generate", 0},
    {"startingAfter",      's',             "STARTING_AFTER",              0, 
        "Look for primes greater than this number", 1},
    {"accuracyFactor",     'a',             "ACCURACY_FACTOR",             0, 
        "If using the probabilistic Miller Rabin \
            algorithm, specify how many iterations\
            should be performed (recommended to use >= 10)", 2},
    {"outfile",            'o',             "FILE_PATH",                   0,
        "Specify path to file to create if you \
            don't want to output to command line.", 3},
    {0}
};

/* Used by main to communicate with parse_opt. */
struct arguments
{
    //char *args[2];                /* numPrimes, startingAfter, accuracyFactor, output*/
    int numPrimes, startingAfter, accuracyFactor;
    char *outfile;
};

/* Parse a single option. */
static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
    /* Get the input argument from argp_parse, which we
       know is a pointer to our arguments structure. */
    struct arguments *arguments = state->input;

    switch (key)
          {
          case 'n':
            arguments->numPrimes = atoi(arg);
            break;
          case 's':
            arguments->startingAfter = atoi(arg);
            break;
          case 'a':
            arguments->accuracyFactor = atoi(arg);
            break;
          case 'o':
            arguments->outfile = arg;
            break;
          default:
            return ARGP_ERR_UNKNOWN;
          }
    return 0;
}

/* Our argp parser. */
static struct argp argp = { options, parse_opt, args_doc, doc , 0, 0, 0};

struct FactoredNumber{
    int num;
    int r;
    int d;
};

/////
// CONFIG
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
    struct arguments arguments;
    FILE *outstream;

    static int numPrimes;
    static int accuracyFactor;
    static int startingAfter;
    static bool useDeterministic;

    if (argc == 1) {
        // need to get num primes to generate from stdin
        useDeterministic = getNumPrimesToGenerate(&numPrimes, &startingAfter, &accuracyFactor);
        if (useDeterministic == true) {
            accuracyFactor = -1; // will signal to use deterministic MR algo
        }
        outstream = stdout;
    }
    else {
        /* Set argument defaults */
        arguments.outfile = NULL;
        arguments.numPrimes = 5;
        arguments.startingAfter = 1;
        arguments.accuracyFactor = -1;

        argp_parse(&argp, argc, argv, 0, 0, &arguments);
        
        /* Where do we send output? */
        if (arguments.outfile)
            outstream = fopen (arguments.outfile, "w");
        else
            outstream = stdout;

        numPrimes = arguments.numPrimes;
        startingAfter = arguments.startingAfter;
        accuracyFactor = arguments.startingAfter;
    }
    int *primes = generatePrimes(numPrimes, startingAfter, accuracyFactor);
    int *currentPrime = primes;
    if (arguments.outfile == NULL){
        fprintf(outstream, "Here are the %d primes you requested, starting after %d:\n", numPrimes, startingAfter);
    }
    int j;
    for (int i = 0; i < (numPrimes/10 + numPrimes % 10); i++){
        j = 0;
        while (j++ < 10 && *currentPrime != -1){
            fprintf(outstream, "%10d", *(currentPrime++));
        }
        fprintf(outstream, "\n");
        if (*currentPrime == -1)
            break;
    }
    free(primes);
}

bool getNumPrimesToGenerate(int *numPrimes, int *startingAfter, int *accuracyFactor) {
    char useDeterministic;
    do {
        printf("How many primes shall be generated?\n");
        scanf("%d", numPrimes);
        
        printf("From where shall we start listing primes? (defaults to all primes from 2 onwards)\n");
        scanf("%d", startingAfter);

        printf("Roughly how accurate do you wish this probabilistic approach to be? \n\t N.B. Enter some number greater than 10, so as to reduce\n\tthe chance of falsely identifying a composite number as the next possible prime.\n");
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
    primes = (int *)malloc(numPrimes * sizeof(int) + 1);
    int *currentPrime = primes; // pointer to current index of array of ints
    int *previousPrime;
    // find if startingAfter is prime, or what the first prime following startingAfter is
    *currentPrime = generateNextPrime(startingAfter - 1, accuracyFactor); // start list with first prime
    primes[0] = generateNextPrime(startingAfter - 1, accuracyFactor);
    for (int i = 1; i < numPrimes; i++) {
        previousPrime = currentPrime++; // assign pointer currentPrime to previousPrime, then increment currentPrime's pointer
        *currentPrime = generateNextPrime(*previousPrime, accuracyFactor); // pass value at current pointer index
    }
    *(++currentPrime) = -1;
    return(primes);    
}

int generateNextPrime(int currentPrime, int accuracyFactor) {
    int primeCandidate = currentPrime;
    bool isNotPrime = true;
    do {
        primeCandidate++;
        if (       (!(primeCandidate % 2) && primeCandidate != 2)
                || (!(primeCandidate % 3) && primeCandidate != 3)
                || (!(primeCandidate % 5) && primeCandidate != 5)
                || (!(primeCandidate % 7) && primeCandidate != 7)) {
            continue;
        }
        else {
            if (   primeCandidate == 2 
                || primeCandidate == 3
                || primeCandidate == 5
                || primeCandidate == 7)
                return primeCandidate;
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
