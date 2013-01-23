/*
 *  ofxBulletCustomShape.cpp
 *  ofxBullet Events Example
 *
 *  Created by Nick Hardeman on 7/12/11.
 *  Copyright 2011 Arnold Worldwide. All rights reserved.
 *
 */

#include "ofxBulletCustomShape.h"
#include "ofxBulletConvexDecomposer.h"

//--------------------------------------------------------------
ofxBulletCustomShape::ofxBulletCustomShape() {
	_type = OFX_BULLET_CUSTOM_SHAPE;
}

//--------------------------------------------------------------
ofxBulletCustomShape::~ofxBulletCustomShape() {
	
}

// pass in an already created compound shape //
//--------------------------------------------------------------
void ofxBulletCustomShape::init( btCompoundShape* a_colShape, ofVec3f a_centroid ) {
	_shape		= (btCollisionShape*)a_colShape;
	_centroid	= a_centroid;
	_bInited	= true;
}

//--------------------------------------------------------------
void ofxBulletCustomShape::create( btDynamicsWorld* a_world, ofVec3f a_loc, float a_mass ) {
	btTransform tr = ofGetBtTransformFromVec3f( a_loc );
	create( a_world, tr, a_mass );
}

//--------------------------------------------------------------
void ofxBulletCustomShape::create( btDynamicsWorld* a_world, ofVec3f a_loc, ofQuaternion a_rot, float a_mass ) {
	btTransform tr	= ofGetBtTransformFromVec3f( a_loc );
	tr.setRotation( btQuaternion(btVector3(a_rot.x(), a_rot.y(), a_rot.z()), a_rot.w()) );
	create( a_world, tr, a_mass );
}

//--------------------------------------------------------------
void ofxBulletCustomShape::create( btDynamicsWorld* a_world, btTransform a_bt_tr, float a_mass ) {
	_world = a_world;
	_mass = a_mass;
	if(!_bInited) {
		_shape = new btCompoundShape(false);
		_centroid.set(0, 0, 0);
	}
	_startTrans		= a_bt_tr;
	_bCreated		= true;
}

//--------------------------------------------------------------
bool ofxBulletCustomShape::addShape( btCollisionShape* a_colShape, ofVec3f a_localCentroidPos ) {
	if(_bAdded == true) {
		ofLog( OF_LOG_ERROR, "ofxBulletCustomShape :: addShape : can not call after calling add()" );
		return false;
	}
	shapes.push_back( a_colShape );
	centroids.push_back( a_localCentroidPos );
	return true;
}

//--------------------------------------------------------------
bool ofxBulletCustomShape::addMesh( ofMesh a_mesh, ofVec3f a_localScaling, bool a_bUseConvexHull, bool centerVerticesOnCentroid ) {
	if(a_mesh.getMode() != OF_PRIMITIVE_TRIANGLES) {
		ofLog( OF_LOG_ERROR, "ofxBulletCustomShape :: addMesh : mesh must be set to OF_PRIMITIVE_TRIANGLES!! aborting");
		return false;
	}
	if(_bAdded == true) {
		ofLog( OF_LOG_ERROR, "ofxBulletCustomShape :: addMesh : can not call after calling add()" );
		return false;
	}
	
	btVector3 localScaling( a_localScaling.x, a_localScaling.y, a_localScaling.z );
	vector <ofIndexType>	indicies = a_mesh.getIndices();
	vector <ofVec3f>		verticies = a_mesh.getVertices();
	
	btVector3 centroid = btVector3(0, 0, 0);
	
	if(!a_bUseConvexHull) {
		vector<pair<btVector3, btConvexHullShape*> > components = ofxBulletConvexDecomposer::decompose( a_mesh, localScaling );
		btCompoundShape* compound = ofxBulletConvexDecomposer::constructCompoundShape( components );
		
		shapes.push_back( compound );
		centroids.push_back( ofVec3f(0,0,0) );

		//centroids.push_back( ofVec3f(centroid.getX(), centroid.getY(), centroid.getZ()) );
	} else {
		// HULL Building code from example ConvexDecompositionDemo.cpp //
		btTriangleMesh* trimesh = new btTriangleMesh();
		
		for ( int i = 0; i < indicies.size(); i+=3) {
			int index0 = indicies[i+2];
			int index1 = indicies[i+1];
			int index2 = indicies[i];
			
			btVector3 vertex0( verticies[index0].x, verticies[index0].y, verticies[index0].z );
			btVector3 vertex1( verticies[index1].x, verticies[index1].y, verticies[index1].z );
			btVector3 vertex2( verticies[index2].x, verticies[index2].y, verticies[index2].z );
			
			vertex0 *= localScaling;
			vertex1 *= localScaling;
			vertex2 *= localScaling;
			
			trimesh->addTriangle(vertex0, vertex1, vertex2);
		}
		
		//cout << "ofxBulletCustomShape :: addMesh : input triangles = " << trimesh->getNumTriangles() << endl;
		//cout << "ofxBulletCustomShape :: addMesh : input indicies = " << indicies.size() << endl;
		//cout << "ofxBulletCustomShape :: addMesh : input verticies = " << verticies.size() << endl;
		
		btConvexShape* tmpConvexShape = new btConvexTriangleMeshShape(trimesh);
		
		//create a hull approximation
		btShapeHull* hull = new btShapeHull(tmpConvexShape);
		btScalar margin = tmpConvexShape->getMargin();
		hull->buildHull(margin);
		tmpConvexShape->setUserPointer(hull);
		
		centroid = btVector3(0., 0., 0.);
		for (int i = 0; i < hull->numVertices(); i++) {
			centroid += hull->getVertexPointer()[i];
		}
		centroid /= (float)hull->numVertices();
		
		//printf("ofxBulletCustomShape :: addMesh : new hull numTriangles = %d\n", hull->numTriangles());
		//printf("ofxBulletCustomShape :: addMesh : new hull numIndices = %d\n", hull->numIndices());
		//printf("ofxBulletCustomShape :: addMesh : new hull numVertices = %d\n", hull->numVertices());
		
		if ( !centerVerticesOnCentroid )
			centroid = btVector3(0,0,0);
		btConvexHullShape* convexShape = new btConvexHullShape();
		for (int i=0;i<hull->numVertices();i++) {
			convexShape->addPoint(hull->getVertexPointer()[i] - centroid);
		}
		
		delete tmpConvexShape;
		delete hull;
		
		shapes.push_back( convexShape );
		centroids.push_back( ofVec3f(centroid.getX(), centroid.getY(), centroid.getZ()) );
	}
	return true;
}

//--------------------------------------------------------------
void ofxBulletCustomShape::add() {
	_bAdded = true;
	btTransform trans;
	trans.setIdentity();
	
	for(int i = 0; i < centroids.size(); i++) {
		_centroid += centroids[i];
	}
	if(centroids.size() > 0)
		_centroid /= (float)centroids.size();
	btVector3 shiftCentroid;
	for(int i = 0; i < shapes.size(); i++) {
		shiftCentroid = btVector3(centroids[i].x, centroids[i].y, centroids[i].z);
		shiftCentroid -= btVector3(_centroid.x, _centroid.y, _centroid.z);
		trans.setOrigin( ( shiftCentroid ) );
		((btCompoundShape*)_shape)->addChildShape( trans, shapes[i]);
	}
	_rigidBody = ofGetBtRigidBodyFromCollisionShape( _shape, _startTrans, _mass);
	setData( new ofxBulletUserData() );
	_world->addRigidBody(_rigidBody);
	setProperties(.4, .75);
	setDamping( .25 );
}

//--------------------------------------------------------------
ofVec3f ofxBulletCustomShape::getCentroid() {
	return _centroid;
}

//--------------------------------------------------------------
int ofxBulletCustomShape::getNumChildShapes() {
	if(_bAdded) {
		return ((btCompoundShape*)_shape)->getNumChildShapes();
	}
	return 0;
}

//--------------------------------------------------------------
void ofxBulletCustomShape::draw() {
	
}


