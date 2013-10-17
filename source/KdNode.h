#ifndef KDNODE_H
#define KDNODE_H

#include <cstddef>


class KdNode {

	public:
		KdNode( float x, float y, float z );

		float mCoord[3];
		KdNode* mLeft;
		KdNode* mRight;

};

#endif
