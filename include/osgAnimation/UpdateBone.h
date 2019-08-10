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
 */

#ifndef OSGANIMATION_UPDATE_BONE
#define OSGANIMATION_UPDATE_BONE 1

#include <osgAnimation/Export>
#include <osgAnimation/UpdateMatrixTransform>

namespace osgAnimation
{

    class OSGANIMATION_EXPORT UpdateBone : public UpdateMatrixTransform
    {
    public:
        META_Object(osgAnimation, UpdateBone);

        UpdateBone(const std::string& name = "");
        UpdateBone(const UpdateBone&,const osg::CopyOp&);
        void operator()(osg::Node* node, osg::NodeVisitor* nv);
    };

}

#endif
