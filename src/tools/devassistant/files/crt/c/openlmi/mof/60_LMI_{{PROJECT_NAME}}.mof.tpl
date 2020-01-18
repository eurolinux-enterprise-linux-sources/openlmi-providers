/*
 * Copyright (C) 2014 John Doe.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Authors: John Doe <john@example.com>
 */

[ Version("0.0.1"), Provider("cmpi:cmpiLMI_{{ PROJECT_NAME }}"),
  Description("ManagedElement is an abstract class that provides a common "
       "superclass (or top of the inheritance tree) for the "
       "non-association classes in the CIM Schema. Feel free to extend it to your needs. ")]
class LMI_MyCoolClass: CIM_ManagedElement
{
    [ Implemented(true), Override("Caption"),
      Description("The Caption property is a short textual description "
                  "(one- line string) of the object." ),
      MaxLen(64) ]
    string Caption;

    [ Implemented(true), Override("Description"),
      Description("The Description property provides a textual description "
                  "of the object.") ]
    string Description;
};
