#pragma once

#include <map>
#include <string>
#include <vector>

class LineReader;
struct Scene;
struct Template;

/*

scene elements:

	template <name>
		<template-lines>

	entity <name>
		<template-lines>
 
	scene
		nodes
			<node-id-hierarchy>

example scene:

	template A
		transform
			position
				0 0 0
 
	template B
		base A
		transform
			position
				0 2 0
 
	entity E1
		base B
		model
			path
				model.fbx
 
	entity E2
		base B
		model
			path
				model.fbx
 
	entity E3
		base B
		model
			path
				model.fbx
 
	scene
		nodes
			E1
				E2
			E3
*/

bool parseSceneFromLines(
	const TypeDB & typeDB,
	std::vector<std::string> & lines,
	const char * basePath,
	Scene & out_scene);
bool parseSceneFromFile(
	const TypeDB & typeDB,
	const char * path,
	Scene & out_scene);

bool parseSceneObjectFromLines(
	const TypeDB & typeDB,
	LineReader & line_reader,
	Scene & out_scene,
	std::map<std::string, Template> & templates);
bool parseSceneObjectStructureFromLines(
	const TypeDB & typeDB,
	LineReader & line_reader,
	Scene & out_scene,
	std::map<std::string, Template> & templates);
