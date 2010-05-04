#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <getopt.h>

#include "cactus.h"
#include "avl.h"
#include "commonC.h"
#include "hashTableC.h"
#include "hashTableC_itr.h"
#include "bioioC.h"

#include "eval_ComparatorAPI.h"

/*
 * The script takes two MAF files and for each ordered pair of sequences in the MAFS calculates
 * a predefined number of sample homology tests (see below), then reports the statistics in an XML formatted file.
 * It is suitable for running over very large numbers of alignments, because it does not attempt to hold
 * everything in memory, and instead takes a sampling approach.
 *
 * For two sets of pairwise alignments, A and B, a homology test is defined as follows.
 * Pick a pair of aligned positions in A, called a homology pair - the AB homology test returns true if the
 * pair is in B, otherwise it returns false. The set of possible homology tests for the ordered pair (A, B)
 * is not necessarily equivalent to the set of possible (B, A) homology tests.
 * We call the proportion of true tests as a percentage of the total of a set of homology tests C from (A, B)  A~B.
 *
 * If A is the set of true pairwise alignments and B the predicted set of alignments then A~B (over large enough
 * C), is a proxy to sensitivity of B in predicted the set of correctly aligned pairs in A. Conversely B~A (over large enough C)
 * is a proxy to the specificity of the aligned pairs in B with respect to the set of correctly aligned pairs in A.
 */

void usage() {
	fprintf(stderr, "eval_PhyloComparator, version 0.1\n");
	fprintf(stderr, "-a --logLevel : Set the log level\n");
	fprintf(stderr, "-b --mAFFile1 : The location of the first MAF file\n");
	fprintf(stderr, "-c --mAFFile2 : The location of the second MAF file\n");
	fprintf(stderr, "-d --outputFile : The output XML formatted results file.\n");
	fprintf(stderr, "-e --sampleNumber : The number of sample phylotrio tests to perform (total).\n");
	fprintf(stderr, "-x --speciesA: The first species to be compared\n");
	fprintf(stderr, "-y --speciesB: The second species to be compared\n");
	fprintf(stderr, "-z --speciesC: The third species to be compared\n");
	fprintf(stderr, "-h --help : Print this help screen\n");
}

int main(int argc, char *argv[]) {
	/*
	 * Arguments/options
	 */
	char * logLevelString = NULL;
	char * mAFFile1 = NULL;
	char * mAFFile2 = NULL;
	char * outputFile = NULL;
	int32_t sampleNumber = 1000000; // by default do a million samples per pair.
	char * speciesA = NULL;
	char * speciesB = NULL;
	char * speciesC = NULL;

	///////////////////////////////////////////////////////////////////////////
	// (0) Parse the inputs handed by genomeCactus.py / setup stuff.
	///////////////////////////////////////////////////////////////////////////

	while(1) {
		static struct option long_options[] = {
			{ "logLevel", required_argument, 0, 'a' },
			{ "mAFFile1", required_argument, 0, 'b' },
			{ "mAFFile2", required_argument, 0, 'c' },
			{ "outputFile", required_argument, 0, 'd' },
			{ "sampleNumber", required_argument, 0, 'e' },
//			{ "speciesA", required_argument, 0, 'x' },
//			{ "speciesB", required_argument, 0, 'y' },
//			{ "speciesC", required_argument, 0, 'z' },
			{ "speciesA", optional_argument, 0, 'x' },
			{ "speciesB", optional_argument, 0, 'y' },
			{ "speciesC", optional_argument, 0, 'z' },
			{ "help", no_argument, 0, 'h' },
			{ 0, 0, 0, 0 }
		};

		int option_index = 0;

		int key = getopt_long(argc, argv, "a:b:c:d:e:hx:y:z:", long_options, &option_index);

		if(key == -1) {
			break;
		}

		switch(key) {
			case 'a':
				logLevelString = stringCopy(optarg);
				break;
			case 'b':
				mAFFile1 = stringCopy(optarg);
				break;
			case 'c':
				mAFFile2 = stringCopy(optarg);
				break;
			case 'd':
				outputFile = stringCopy(optarg);
				break;
			case 'e':
				assert(sscanf(optarg, "%i", &sampleNumber) == 1);
				break;
			case 'h':
				usage();
				return 0;
			case 'x':
				speciesA = stringCopy(optarg);
				break;
			case 'y':
				speciesB = stringCopy(optarg);
				break;
			case 'z':
				speciesC = stringCopy(optarg);
				break;
			default:
				usage();
				return 1;
		}
	}

	///////////////////////////////////////////////////////////////////////////
	// (0) Check the inputs.
	///////////////////////////////////////////////////////////////////////////

	assert(mAFFile1 != NULL);
	assert(mAFFile2 != NULL);
	assert(outputFile != NULL);
//	assert(speciesA != NULL);
//	assert(speciesB != NULL);
//	assert(speciesC != NULL);

	FILE *fileHandle = fopen(mAFFile1, "r");
	if (fileHandle == NULL) {
		fprintf(stderr, "ERROR, unable to open `%s', is path correct?\n", mAFFile1);
		exit(1);
	}
	fclose(fileHandle);
	fileHandle = fopen(mAFFile2, "r");
	if (fileHandle == NULL) {
		fprintf(stderr, "ERROR, unable to open `%s', is path correct?\n", mAFFile2);
		exit(1);
	}
	fclose(fileHandle);

	//////////////////////////////////////////////
	//Set up logging
	//////////////////////////////////////////////

	if (logLevelString != NULL && strcmp(logLevelString, "INFO") == 0) {
		setLogLevel(LOGGING_INFO);
	}
	if (logLevelString != NULL && strcmp(logLevelString, "DEBUG") == 0) {
		setLogLevel(LOGGING_DEBUG);
	}

	//////////////////////////////////////////////
	//Log (some of) the inputs
	//////////////////////////////////////////////

	logInfo("MAF file 1 name : %s\n", mAFFile1);
	logInfo("MAF file 2 name : %s\n", mAFFile2);
	logInfo("Output stats file : %s\n", outputFile);
	logInfo("Species to be checked: %s, %s, %s\n", speciesA, speciesB, speciesC);

	 //////////////////////////////////////////////
	// Create hashtable for the first MAF file.
	//////////////////////////////////////////////

	struct hashtable *seqNameHash;
	seqNameHash = create_hashtable(256, hashtable_stringHashKey, hashtable_stringEqualKey, NULL, NULL);
	populateNameHash(mAFFile1, seqNameHash);

	// TODO: Check if query species are in maf file

	//////////////////////////////////////////////
	//Do comparisons.
	//////////////////////////////////////////////

	struct avl_table *results_12 = compareMAFs_AB_Trio(mAFFile1, mAFFile2, sampleNumber, seqNameHash, speciesA, speciesB, speciesC);
	struct avl_table *results_21 = compareMAFs_AB_Trio(mAFFile2, mAFFile1, sampleNumber, seqNameHash, speciesA, speciesB, speciesC);

	fileHandle = fopen(outputFile, "w");
	if (fileHandle == NULL) {
		fprintf(stderr, "ERROR, unable to open `%s' for writing.\n", outputFile);
		exit(1);
	}

	fprintf(fileHandle, "<trio_comparisons sampleNumber=\"%i\">\n", sampleNumber);
	reportResultsTrio(results_12, mAFFile1, mAFFile2, fileHandle);
	reportResultsTrio(results_21, mAFFile2, mAFFile1, fileHandle);
	fprintf(fileHandle, "</trio_comparisons>\n");
	fclose(fileHandle);

	///////////////////////////////////////////////////////////////////////////
	// Clean up.
	///////////////////////////////////////////////////////////////////////////

	avl_destroy(results_12, (void (*)(void *, void *))aPair_destruct);
	avl_destroy(results_21, (void (*)(void *, void *))aPair_destruct);

	return 0;
}