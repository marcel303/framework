/*
	Copyright (C) 2020 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

struct Graph_TypeDefinitionLibrary;

struct AudioEnumTypeRegistration;
struct AudioNodeTypeRegistration;

// -- functions for adding individual types

void createAudioValueTypeDefinitions(
	Graph_TypeDefinitionLibrary & typeDefinitionLibrary);

void createAudioEnumTypeDefinitions(
	Graph_TypeDefinitionLibrary & typeDefinitionLibrary,
	const AudioEnumTypeRegistration * registrationList);

void createAudioNodeTypeDefinitions(
	Graph_TypeDefinitionLibrary & typeDefinitionLibrary,
	const AudioNodeTypeRegistration * registrationList);

// -- create types using custom type registration lists

void createAudioTypeDefinitionLibrary(
	Graph_TypeDefinitionLibrary & typeDefinitionLibrary,
	const AudioEnumTypeRegistration * enumRegistrationList,
	const AudioNodeTypeRegistration * nodeRegistrationList);

// -- create types using the standard type definition lists

void createAudioTypeDefinitionLibrary(
	Graph_TypeDefinitionLibrary & typeDefinitionLibrary);

Graph_TypeDefinitionLibrary * createAudioTypeDefinitionLibrary(); // for convenience
