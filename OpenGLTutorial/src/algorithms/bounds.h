#ifndef BOUNDS_H
#define BOUNDS_H

#include <glm/glm.hpp>

enum class BoundTypes : unsigned char {
	AABB = 0x00,	// 0x00 = 0	// Axis-aligned bounding box
	SPHERE = 0x01	// 0x01 = 1
};

class BoundingRegion {
public:
	BoundTypes type;

	// sphere values
	glm::vec3 center;
	float radius;

	// bounding box values
	glm::vec3 min;
	glm::vec3 max;

	BoundingRegion(BoundTypes type);

	// initialize as sphere
	BoundingRegion(glm::vec3 center, float radius);

	// initialize as AABB
	BoundingRegion(glm::vec3 min, glm::vec3 max);

	glm::vec3 calculateCenter();
	glm::vec3 calculateDimensions();

	
	bool containsPoint(glm::vec3 pt);
	bool containsRegion(BoundingRegion br);
	bool intersectsWith(BoundingRegion br);
};

#endif