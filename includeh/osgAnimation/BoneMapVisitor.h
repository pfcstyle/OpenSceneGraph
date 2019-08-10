/*  -*-c++-*-
 *  Copyright (C) 2009 Cedric Pinson <cedric.pinson@plopbyte.net>
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
 *
 * Authors:
 *         Cedric Pinson <cedric.pinson@plopbyte.net>
 */

#ifndef OSGANIMATION_BONEMAP_VISITOR
#define OSGANIMATION_BONEMAP_VISITOR 1

#include <osgAnimation/Export>
#include <osgAnimation/Bone>
#include <osg/NodeVisitor>

namespace osgAnimation
{
    class OSGANIMATION_EXPORT BoneMapVisitor : public osg::NodeVisitor
    {
    public:
        META_NodeVisitor(osgAnimation, BoneMapVisitor)
        BoneMapVisitor();

        void apply(osg::Node&);
        void apply(osg::Transform& node);
        const BoneMap& getBoneMap() const;

    protected:
        BoneMap _map;
    };
}

#endif
