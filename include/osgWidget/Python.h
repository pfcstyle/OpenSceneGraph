/* -*-c++-*- OpenSceneGraph - Copyright (C) 1998-2008 Robert Osfield
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

// Code by: Jeremy Moles (cubicool) 2007-2008

#ifndef OSGWIDGET_PYTHON
#define OSGWIDGET_PYTHON

#include <osgWidget/WindowManager>

namespace osgWidget {

struct PythonEngineData;

class OSGWIDGET_EXPORT PythonEngine: public ScriptEngine {
    public:
        PythonEngine(WindowManager* = 0);

        bool initialize ();
        bool close      ();
        bool eval       (const std::string&);
        bool runFile    (const std::string&);

    protected:

        PythonEngineData* _data;
        WindowManager*    _wm;
};

}

#endif
