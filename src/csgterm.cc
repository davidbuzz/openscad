/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2011 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "csgterm.h"
#include "polyset.h"
#include <sstream>

/*!
	\class CSGTerm

	A CSGTerm is either a "primitive" or a CSG operation with two
	children terms. A primitive in this context is any PolySet, which
	may or may not have a subtree which is already evaluated (e.g. using
	the render() module).

 */

/*!
	\class CSGChain

	A CSGChain is just a vector of primitives, each having a CSG type associated with it.
	It's created by importing a CSGTerm tree.

 */


CSGTerm::CSGTerm(const shared_ptr<PolySet> &polyset, const Transform3d &matrix, const double color[4], const std::string &label)
	: type(TYPE_PRIMITIVE), polyset(polyset), label(label)
{
	this->m = matrix;
	for (int i = 0; i < 4; i++) this->color[i] = color[i];
}

CSGTerm::CSGTerm(type_e type, shared_ptr<CSGTerm> left, shared_ptr<CSGTerm> right)
	: type(type), left(left), right(right)
{
}

CSGTerm::CSGTerm(type_e type, CSGTerm *left, CSGTerm *right)
	: type(type), left(left), right(right)
{
}

CSGTerm::~CSGTerm()
{
}


shared_ptr<CSGTerm> CSGTerm::normalize(shared_ptr<CSGTerm> &term)
{
	// This function implements the CSG normalization
	// Reference: Florian Kirsch, Juergen Doeller,
	// OpenCSG: A Library for Image-Based CSG Rendering,
	// University of Potsdam, Hasso-Plattner-Institute, Germany
	// http://www.opencsg.org/data/csg_freenix2005_paper.pdf

	if (term->type == TYPE_PRIMITIVE) return term;

	shared_ptr<CSGTerm> x = normalize(term->left);
	shared_ptr<CSGTerm> y = normalize(term->right);

	shared_ptr<CSGTerm> t1(term);
	if (x != term->left || y != term->right) t1.reset(new CSGTerm(term->type, x, y));

	shared_ptr<CSGTerm> t2;
	while (1) {
		t2 = normalize_tail(t1);
		if (t1 == t2)	break;
		t1 = t2;
	}

	return t1;
}

shared_ptr<CSGTerm> CSGTerm::normalize_tail(shared_ptr<CSGTerm> &term)
{
	// Part A: The 'x . (y . z)' expressions

	shared_ptr<CSGTerm> x = term->left;
	shared_ptr<CSGTerm> y = term->right->left;
	shared_ptr<CSGTerm> z = term->right->right;

	CSGTerm *result = NULL;

	// 1.  x - (y + z) -> (x - y) - z
	if (term->type == TYPE_DIFFERENCE && term->right->type == TYPE_UNION) {
		result = new CSGTerm(TYPE_DIFFERENCE, 
												 shared_ptr<CSGTerm>(new CSGTerm(TYPE_DIFFERENCE, x, y)),
												 z);
	}
	// 2.  x * (y + z) -> (x * y) + (x * z)
	else if (term->type == TYPE_INTERSECTION && term->right->type == TYPE_UNION) {
		result = new CSGTerm(TYPE_UNION, 
												 new CSGTerm(TYPE_INTERSECTION, x, y), 
												 new CSGTerm(TYPE_INTERSECTION, x, z));
	}
	// 3.  x - (y * z) -> (x - y) + (x - z)
	else if (term->type == TYPE_DIFFERENCE && term->right->type == TYPE_INTERSECTION) {
		result = new CSGTerm(TYPE_UNION, 
												 new CSGTerm(TYPE_DIFFERENCE, x, y), 
												 new CSGTerm(TYPE_DIFFERENCE, x, z));
	}
	// 4.  x * (y * z) -> (x * y) * z
	else if (term->type == TYPE_INTERSECTION && term->right->type == TYPE_INTERSECTION) {
		result = new CSGTerm(TYPE_INTERSECTION, 
												 shared_ptr<CSGTerm>(new CSGTerm(TYPE_INTERSECTION, x, y)),
												 z);
	}
	// 5.  x - (y - z) -> (x - y) + (x * z)
	else if (term->type == TYPE_DIFFERENCE && term->right->type == TYPE_DIFFERENCE) {
		result = new CSGTerm(TYPE_UNION, 
												 new CSGTerm(TYPE_DIFFERENCE, x, y), 
												 new CSGTerm(TYPE_INTERSECTION, x, z));
	}
	// 6.  x * (y - z) -> (x * y) - z
	else if (term->type == TYPE_INTERSECTION && term->right->type == TYPE_DIFFERENCE) {
		result = new CSGTerm(TYPE_DIFFERENCE, 
												 shared_ptr<CSGTerm>(new CSGTerm(TYPE_INTERSECTION, x, y)),
												 z);
	}
	if (result) return shared_ptr<CSGTerm>(result);

	// Part B: The '(x . y) . z' expressions

	x = term->left->left;
	y = term->left->right;
	z = term->right;

	// 7. (x - y) * z  -> (x * z) - y
	if (term->left->type == TYPE_DIFFERENCE && term->type == TYPE_INTERSECTION) {
		result = new CSGTerm(TYPE_DIFFERENCE, 
												 shared_ptr<CSGTerm>(new CSGTerm(TYPE_INTERSECTION, x, z)), 
												 y);
	}
	// 8. (x + y) - z  -> (x - z) + (y - z)
	else if (term->left->type == TYPE_UNION && term->type == TYPE_DIFFERENCE) {
		result = new CSGTerm(TYPE_UNION, 
												 new CSGTerm(TYPE_DIFFERENCE, x, z), 
												 new CSGTerm(TYPE_DIFFERENCE, y, z));
	}
	// 9. (x + y) * z  -> (x * z) + (y * z)
	else if (term->left->type == TYPE_UNION && term->type == TYPE_INTERSECTION) {
		result = new CSGTerm(TYPE_UNION, 
												 new CSGTerm(TYPE_INTERSECTION, x, z), 
												 new CSGTerm(TYPE_INTERSECTION, y, z));
	}

	if (result) return shared_ptr<CSGTerm>(result);
	
	return term;
}

std::string CSGTerm::dump()
{
	std::stringstream dump;

	if (type == TYPE_UNION)
		dump << "(" << left->dump() << " + " << right->dump() << ")";
	else if (type == TYPE_INTERSECTION)
		dump << "(" << left->dump() << " * " << right->dump() << ")";
	else if (type == TYPE_DIFFERENCE)
		dump << "(" << left->dump() << " - " << right->dump() << ")";
	else 
		dump << this->label;

	return dump.str();
}

CSGChain::CSGChain()
{
}

void CSGChain::add(const shared_ptr<PolySet> &polyset, const Transform3d &m, double *color, CSGTerm::type_e type, std::string label)
{
	polysets.push_back(polyset);
	matrices.push_back(m);
	colors.push_back(color);
	types.push_back(type);
	labels.push_back(label);
}

void CSGChain::import(shared_ptr<CSGTerm> term, CSGTerm::type_e type)
{
	if (term->type == CSGTerm::TYPE_PRIMITIVE) {
		add(term->polyset, term->m, term->color, type, term->label);
	} else {
		import(term->left, type);
		import(term->right, term->type);
	}
}

std::string CSGChain::dump()
{
	std::stringstream dump;

	for (size_t i = 0; i < types.size(); i++)
	{
		if (types[i] == CSGTerm::TYPE_UNION) {
			if (i != 0) dump << "\n";
			dump << "+";
		}
		else if (types[i] == CSGTerm::TYPE_DIFFERENCE)
			dump << " -";
		else if (types[i] == CSGTerm::TYPE_INTERSECTION)
			dump << " *";
		dump << labels[i];
	}
	dump << "\n";
	return dump.str();
}

BoundingBox CSGChain::getBoundingBox() const
{
	BoundingBox bbox;
	for (size_t i=0;i<polysets.size();i++) {
		if (types[i] != CSGTerm::TYPE_DIFFERENCE) {
			BoundingBox psbox = polysets[i]->getBoundingBox();
			if (!psbox.isNull()) {
				Eigen::Transform3d t;
				// Column-major vs. Row-major
				t = matrices[i];
				bbox.extend(t * psbox.min());
				bbox.extend(t * psbox.max());
			}
		}
	}
	return bbox;
}
