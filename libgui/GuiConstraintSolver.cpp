#include "libgui_precompiled.h"
#include "GuiConstraintSolver.h"
#include "Widget.h"

namespace Gui
{
	// Helper function to do an STL sort on a child list that sorts children by alignment index.
	static bool CompareAlignmentIndices(const Widget* widget1, const Widget* widget2)
	{
		const int alignmentIndex1 = widget1->GetAlignmentIndex();
		const int alignmentIndex2 = widget1->GetAlignmentIndex();

		if (alignmentIndex1 != alignmentIndex2)
			return alignmentIndex1 < alignmentIndex2;

		return widget1 < widget2;
	}

	static Point CalculateConstrainedSize(const Widget* widget, const Point& size)
	{
		Point result = size;

		const Point minSize = widget->GetMinSizeConstraint();
		const Point maxSize = widget->GetMaxSizeConstraint();

		if (maxSize.x != 0)
		{
			if (result.x > maxSize.x)
				result.x = maxSize.x;
		}
		if (maxSize.y != 0)
		{
			if (result.y > maxSize.y)
				result.y = maxSize.y;
		}

		if (minSize.x != 0)
		{
			if (result.x < minSize.x)
				result.x = minSize.x;
		}
		if (minSize.y != 0)
		{
			if (result.y < minSize.y)
				result.y = minSize.y;
		}

		return result;
	}

	static Point CalculateConstrainedSize(const Widget* widget, const Point& size, bool overrideX, bool overrideY)
	{
		Point result;

		result = CalculateConstrainedSize(widget, size);

		if (overrideX)
			result.x = size.x;
		if (overrideY)
			result.y = size.y;

		return result;
	}

	ConstraintSolver& ConstraintSolver::I()
	{
		static ConstraintSolver solver;
		return solver;
	}

	Point ConstraintSolver::GetConstrainedSize(const Widget* widget, const Point& size)
	{
		return CalculateConstrainedSize(widget, size, false, false);
	}

	void ConstraintSolver::Align(Widget* container)
	{
		const Area containerArea = container->GetArea();
		Widget::ChildList children = container->GetChildList();

		std::sort(children.begin(), children.end(), CompareAlignmentIndices);

		int startL = 0;
		int startR = 0;
		int startT = 0;
		int startB = 0;

		// Top.
		for (size_t i = 0; i < children.size(); ++i)
		{
			if (children[i]->GetAlignment() == Alignment_Top)
			{
				Widget* temp = children[i];
				temp->SetSize(containerArea.size.x, temp->GetHeight());
				temp->SetPosition(0, startT);
				startT += temp->GetHeight();
			}
		}

		// Bottom.
		for (size_t i = 0; i < children.size(); ++i)
		{
			if (children[i]->GetAlignment() == Alignment_Bottom)
			{
				Widget* temp = children[i];
				temp->SetSize(containerArea.size.x, temp->GetHeight());
				temp->SetPosition(0, containerArea.size.y - startB - temp->GetHeight());
				startB += temp->GetHeight();
			}
		}

		// Left.
		for (size_t i = 0; i < children.size(); ++i)
		{
			if (children[i]->GetAlignment() == Alignment_Left)
			{
				Widget* temp = children[i];
				temp->SetSize(temp->GetWidth(), containerArea.size.y - startT - startB);
				temp->SetPosition(startL, startT);
				startL += temp->GetWidth();
			}
		}
		
		// Right.
		for (size_t i = 0; i < children.size(); ++i)
		{
			if (children[i]->GetAlignment() == Alignment_Right)
			{
				Widget* temp = children[i];
				temp->SetSize(temp->GetWidth(), containerArea.size.y - startT - startB);
				temp->SetPosition(containerArea.size.x - startR - temp->GetWidth(), startT);
				startR += temp->GetWidth();
			}
		}
		
		// Client.
		for (size_t i = 0; i < children.size(); ++i)
		{
			if (children[i]->GetAlignment() == Alignment_Client)
			{
				Widget* temp = children[i];
				temp->SetSize(containerArea.size.x - startL - startR, containerArea.size.y - startT - startB);
				temp->SetPosition(startL, startT);
			}
		}

		// Anchors.
		for (size_t i = 0; i < children.size(); ++i)
		{
			if (children[i]->GetAlignment() == Alignment_None)
			{
				Widget* parent = children[i]->GetParent();
				Widget* child = children[i];

				int anchorMask = child->GetAnchorMask();

				if (parent != 0 && anchorMask != 0)
				{
					Area area = child->GetArea();
					Area parentArea = parent->GetClientArea();
					Rect anchors = child->GetAnchors();

					if (anchorMask & Anchor_Left)
					{
						int delta = area.position.x - anchors.min.x;
						area.position.x -= delta;
					}

					if (anchorMask & Anchor_Right)
					{
						int delta = parentArea.size.x - (area.position.x + area.size.x) - anchors.max.x;
						if ((anchorMask & Anchor_Left) == 0)
							area.position.x += delta;
						else
							area.size.x += delta;
					}

					if (anchorMask & Anchor_Top)
					{
						int delta = area.position.y - anchors.min.y;
						area.position.y -= delta;
					}

					if (anchorMask & Anchor_Bottom)
					{
						int delta = parentArea.size.y - (area.position.y + area.size.y) - anchors.max.y;
						if ((anchorMask & Anchor_Top) == 0)
							area.position.y += delta;
						else
							area.size.y += delta;
					}

					child->SetPosition(area.position.x, area.position.y);
					child->SetSize(area.size.x, area.size.y);
				}
			}
		}
	}

	ConstraintSolver::ConstraintSolver()
	{
		// NOP.
	}
};
