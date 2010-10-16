#ifndef malnSet_h
#define malnSet_h
#include "sonLibTypes.h"
struct Genomes;
struct Genome;
struct malnSet;
struct malnBlk;
struct malnComp;
struct Seq;
struct malnBlkMap;

/* construct an empty malnSet  */
struct malnSet *malnSet_construct(struct Genomes *genomes);

/* Construct a malnSet from a MAF file. defaultBranchLength is used to
 * assign branch lengths when inferring trees from the MAF. */
struct malnSet *malnSet_constructFromMaf(struct Genomes *genomes, char *mafFileName, double defaultBranchLength, struct Genome *treelessRootGenome);

/* destructor */
void malnSet_destruct(struct malnSet *malnSet);

/* get associated genomes object  */
struct Genomes *malnSet_getGenomes(struct malnSet *malnSet);

/* add a block to a malnSet */
void malnSet_addBlk(struct malnSet *malnSet, struct malnBlk *blk);

/* remove a single component from malnSet range map */
void malnSet_removeComp(struct malnSet *malnSet, struct malnComp *comp);

/* remove a block from malnSet */
void malnSet_removeBlk(struct malnSet *malnSet, struct malnBlk *blk);

/* remove a block from malnSet and free the block */
void malnSet_deleteBlk(struct malnSet *malnSet, struct malnBlk *blk);

/* get iterator of the blocks. Don't remove or add blocks while in motion. */
struct malnBlkMapIterator *malnSet_getBlocks(struct malnSet *malnSet);

/* Get a list of components that overlap the specified reference range and are
 * in blocks not flagged as done and passing treeLoc filters.  Return NULL if
 * no overlaps. */
stList *malnSet_getOverlappingPendingComps(struct malnSet *malnSet, struct Seq *seq, int chromStart, int chromEnd, unsigned treeLocFilter);


/* Get a list of components that overlaps or are adjacent to the specified
 * reference range and are in blocks not flagged as done and passing treeLoc
 * filters.  Return NULL if no overlaps. */
stList *malnSet_getOverlappingAdjacentPendingComps(struct malnSet *malnSet, struct Seq *seq, int chromStart, int chromEnd, unsigned treeLocFilter);

/* assert some sanity checks on a set */
void malnSet_assert(struct malnSet *malnSet);

/* assert done flag is set on all blocks */
void malnSet_assertDone(struct malnSet *malnSet);

/* clear done flag on all blocks */
void malnSet_clearDone(struct malnSet *malnSet);

/* write a malnSet to a MAF file  */
void malnSet_writeMaf(struct malnSet *malnSet, char *mafFileName);

/* print set for debugging */
void malnSet_dump(struct malnSet *malnSet, const char *label, FILE *fh);

#endif
