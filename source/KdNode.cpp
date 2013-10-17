#include "KdNode.h"


KdNode::KdNode( float x, float y, float z ) {
	mCoord[0] = x;
	mCoord[1] = y;
	mCoord[2] = z;
	mLeft = NULL;
	mRight = NULL;
}
