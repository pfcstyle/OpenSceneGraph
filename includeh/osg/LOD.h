/* -*-c++-*- OpenSceneGraph - Copyright (C) 1998-2006 Robert Osfield
 *
 * This library is open source and may be redistributed and/or modified under
 * the terms of the OpenSceneGraph Public License (OSGPL) version 0.0 or
 * (at your option) any later version.  The full license is in LICENSE file
 * included with this distribution, and on the openscenegraph.org website.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * OpenSceneGraph Public License for more details.
*/

#ifndef OSG_LOD
#define OSG_LOD 1

#include <osg/Group>

namespace osg {

/** LOD - Level Of Detail group node which allows switching between children
    depending on distance from eye point.
    Typical uses are for load balancing - objects further away from
    the eye point are rendered at a lower level of detail, and at times
    of high stress on the graphics pipeline lower levels of detail can
    also be chosen by adjusting the viewers's Camera/CullSettings LODScale value.
    Each child has a corresponding valid range consisting of a minimum
    and maximum distance. Given a distance to the viewer (d), LOD displays
    a child if min <= d < max. LOD may display multiple children simultaneously
    if their corresponding ranges overlap. Children can be in any order,
    and don't need to be sorted by range or amount of detail. If the number of
    ranges (m) is less than the number of children (n), then children m+1 through
    n are ignored.
*/
class OSG_EXPORT LOD : public Group
{
    public :

        /** Default constructor
          * The default constructor sets
          *  - the center mode to USE_BOUNDING_SPHERE,
          *  - the radius to a value smaller than zero and
          *  - the range mode to DISTANCE_FROM_EYE_POINT. */
        LOD();

        /** Copy constructor using CopyOp to manage deep vs shallow copy.*/
        LOD(const LOD&,const CopyOp& copyop=CopyOp::SHALLOW_COPY);

        META_Node(osg, LOD);

        typedef osg::BoundingSphere::vec_type vec_type;
        typedef osg::BoundingSphere::value_type value_type;

        virtual void traverse(NodeVisitor& nv);

        using osg::Group::addChild;

        virtual bool addChild(Node *child);

        virtual bool addChild(Node *child, float rmin, float rmax);

        template<class T> bool addChild( const ref_ptr<T>& child, float rmin, float rmax) { return addChild(child.get(), rmin, rmax); }

        virtual bool removeChildren(unsigned int pos,unsigned int numChildrenToRemove=1);

        typedef std::pair<float,float>  MinMaxPair;
        typedef std::vector<MinMaxPair> RangeList;

        /** Modes which control how the center of object should be determined when computing which child is active.
          * Furthermore it determines how the bounding sphere is calculated. */
        enum CenterMode
        {
            USE_BOUNDING_SPHERE_CENTER, ///< Uses the bounding sphere's center as the center of object and the geometrical bounding sphere of the node's children
            USER_DEFINED_CENTER, ///< Uses the user defined center as the center of object; the bounding sphere is determined by the user defined center and user defined radius
            UNION_OF_BOUNDING_SPHERE_AND_USER_DEFINED ///< Uses the user defined center as the center of object; the bounding sphere is the user defined bounding sphere expanded by the geometrical bounding sphere of the node's children
        };

        /** Set how the center of object should be determined when computing which child is active.*/
        void setCenterMode(CenterMode mode) { _centerMode=mode; }

        /** Get how the center of object should be determined when computing which child is active.*/
        CenterMode getCenterMode() const { return _centerMode; }

        /** Sets the object-space point which defines the center of the osg::LOD.
          * center is affected by any transforms in the hierarchy above the osg::LOD.
          * @note This method also changes the center mode to USER_DEFINED_CENTER
          *       if the current center mode does not use a user defined center!
          * @sa setRadius */
        inline void setCenter(const vec_type& center) { if (_centerMode!=UNION_OF_BOUNDING_SPHERE_AND_USER_DEFINED) { _centerMode=USER_DEFINED_CENTER; } _userDefinedCenter = center; }

        /** return the LOD center point. */
        inline const vec_type& getCenter() const { if ((_centerMode==USER_DEFINED_CENTER)||(_centerMode==UNION_OF_BOUNDING_SPHERE_AND_USER_DEFINED)) return _userDefinedCenter; else return getBound().center(); }


        /** Set the object-space reference radius of the volume enclosed by the LOD.
          * @param radius Radius must be larger or equal to zero. If the radius is smaller than zero the geometrical bounding sphere
          *               of the node's children is used as the LOD's bounding sphere regardless of the center mode setting.
          * @remark The radius is only used to calculate the bounding sphere.
          * @sa setCenter */
        inline void setRadius(value_type radius) { _radius = radius; }

        /** Get the object-space radius of the volume enclosed by the LOD.*/
        inline value_type getRadius() const { return _radius; }

        /** Modes that control how the range values should be interpreted when computing which child is active.*/
        enum RangeMode
        {
            DISTANCE_FROM_EYE_POINT,
            PIXEL_SIZE_ON_SCREEN
        };

        /** Set how the range values should be interpreted when computing which child is active.*/
        void setRangeMode(RangeMode mode) { _rangeMode = mode; }

        /** Get how the range values should be interpreted when computing which child is active.*/
        RangeMode getRangeMode() const { return _rangeMode; }


        /** Sets the min and max visible ranges of range of specific child.
            Values are floating point distance specified in local objects coordinates.*/
        void setRange(unsigned int childNo, float min,float max);

        /** returns the min visible range for specified child.*/
        inline float getMinRange(unsigned int childNo) const { return _rangeList[childNo].first; }

        /** returns the max visible range for specified child.*/
        inline float getMaxRange(unsigned int childNo) const { return _rangeList[childNo].second; }

        /** returns the number of ranges currently set.
          * An LOD which has been fully set up will have getNumChildren()==getNumRanges(). */
        inline unsigned int getNumRanges() const { return static_cast<unsigned int>(_rangeList.size()); }

        /** set the list of MinMax ranges for each child.*/
        inline void setRangeList(const RangeList& rangeList) { _rangeList=rangeList; }

        /** return the list of MinMax ranges for each child.*/
        inline const RangeList& getRangeList() const { return _rangeList; }

        virtual BoundingSphere computeBound() const;

    protected :
        virtual ~LOD() {}

        CenterMode                      _centerMode;
        vec_type                        _userDefinedCenter;
        value_type                      _radius;

        RangeMode                       _rangeMode;
        RangeList                       _rangeList;

};

}

#endif
