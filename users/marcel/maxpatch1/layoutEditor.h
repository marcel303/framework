#pragma once

namespace ControlSurfaceDefinition
{
	struct Element;
	struct ElementLayout;
	struct Surface;
	struct SurfaceLayout;
	
	struct LayoutEditor
	{
		static const int kCornerSize = 7;
		static const int kSnapSize = 8;
		static const int kPaddingSize = 4;
		
		enum State
		{
			kState_Idle,
			kState_DragMove,
			kState_DragSize
		};
		
		const Surface * surface = nullptr;
		
		SurfaceLayout * layout = nullptr;
		
		State state = kState_Idle;
		
		ElementLayout * selected_element = nullptr;
		int drag_offset_x = 0; // for positioning and sizing
		int drag_offset_y = 0;
		int drag_direction_x = 0;
		int drag_direction_y = 0;
		int drag_corner_x = 0; // for sizing only
		int drag_corner_y = 0;
		
		bool has_snap_x = false;
		bool has_snap_y = false;
		int snap_x = 0;
		int snap_y = 0;
		
		LayoutEditor(const Surface * in_surface, SurfaceLayout * in_layout);
		
		const Element * findSurfaceElement(const char * groupName, const char * name) const;
		
		void getElementPositionAndSize(const char * groupName, const char * name, int & x, int & y, int & sx, int & sy) const;
		
		void snapToSurfaceElements(
			ElementLayout & layout_elem,
			const int snap_direction_x,
			bool & has_snap_x,
			int & snap_x,
			const int snap_direction_y,
			bool & has_snap_y,
			int & snap_y) const;
		
		void sizeConstrain(
			ElementLayout & layout_elem_to_skip,
			int & x1,
			int & y1,
			int & x2,
			int & y2,
			const int snap_direction_x, // -1, 0, +1
			bool & has_snap_x,
			int & snap_x,
			const int snap_direction_y, // -1, 0, +1
			bool & has_snap_y,
			int & snap_y) const;
		
		static void dragSize(
			int & elem_x,
			int & elem_sx,
			const int mouse_x,
			const int drag_corner_x,
			const int min_size);
		
		bool tick(const float dt, bool & inputIsCaptured, const bool enableEditing, const bool enableSnapping);
		
		void drawOverlay(const bool enableEditing) const;
	};
}
