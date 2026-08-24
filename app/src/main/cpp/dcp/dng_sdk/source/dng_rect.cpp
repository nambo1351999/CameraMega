

#include "dng_rect.h"

#include "dng_utils.h"

bool dng_rect::operator== (const dng_rect &rect) const
	{
	
	return (rect.t == t) &&
		   (rect.l == l) &&
		   (rect.b == b) &&
		   (rect.r == r);
		   
	}
			

bool dng_rect::IsZero () const
	{

	return (t == 0) && (l == 0) && (b == 0) && (r == 0);

	}
			

bool dng_rect_real64::operator== (const dng_rect_real64 &rect) const
	{
	
	return (rect.t == t) &&
		   (rect.l == l) &&
		   (rect.b == b) &&
		   (rect.r == r);
		   
	}
			

bool dng_rect_real64::IsZero () const
	{

	return (t == 0.0) && (l == 0.0) && (b == 0.0) && (r == 0.0);

	}
			

dng_rect operator& (const dng_rect &a,
					const dng_rect &b)
	{
	
	dng_rect c;
	
	c.t = Max_int32 (a.t, b.t);
	c.l = Max_int32 (a.l, b.l);
	
	c.b = Min_int32 (a.b, b.b);
	c.r = Min_int32 (a.r, b.r);
	
	if (c.IsEmpty ())
		{
		
		c = dng_rect ();
		
		}
		
	return c;
	
	}

dng_rect operator| (const dng_rect &a,
					const dng_rect &b)
	{
	
	if (a.IsEmpty ())
		{
		return b;
		}
		
	if (b.IsEmpty ())
		{
		return a;
		}
		
	dng_rect c;
	
	c.t = Min_int32 (a.t, b.t);
	c.l = Min_int32 (a.l, b.l);
	
	c.b = Max_int32 (a.b, b.b);
	c.r = Max_int32 (a.r, b.r);
	
	return c;
	
	}

dng_rect_real64 operator& (const dng_rect_real64 &a,
						   const dng_rect_real64 &b)
	{
	
	dng_rect_real64 c;
	
	c.t = Max_real64 (a.t, b.t);
	c.l = Max_real64 (a.l, b.l);
	
	c.b = Min_real64 (a.b, b.b);
	c.r = Min_real64 (a.r, b.r);
	
	if (c.IsEmpty ())
		{
		
		c = dng_rect_real64 ();
		
		}
		
	return c;
	
	}

dng_rect_real64 operator| (const dng_rect_real64 &a,
						   const dng_rect_real64 &b)
	{
	
	if (a.IsEmpty ())
		{
		return b;
		}
		
	if (b.IsEmpty ())
		{
		return a;
		}
		
	dng_rect_real64 c;
	
	c.t = Min_real64 (a.t, b.t);
	c.l = Min_real64 (a.l, b.l);
	
	c.b = Max_real64 (a.b, b.b);
	c.r = Max_real64 (a.r, b.r);
	
	return c;
	
	}

dng_rect_real64 Bounds (const dng_point_real64 &a,
						const dng_point_real64 &b,
						const dng_point_real64 &c,
						const dng_point_real64 &d)
	{
									
	real64 xMin = Min_real64 (a.h, Min_real64 (b.h, Min_real64 (c.h, d.h)));
	real64 xMax = Max_real64 (a.h, Max_real64 (b.h, Max_real64 (c.h, d.h)));

	real64 yMin = Min_real64 (a.v, Min_real64 (b.v, Min_real64 (c.v, d.v)));
	real64 yMax = Max_real64 (a.v, Max_real64 (b.v, Max_real64 (c.v, d.v)));

	return dng_rect_real64 (yMin,
							xMin,
							yMax,
							xMax);
									
	}

bool Intersect (const dng_oriented_bounding_box &aBox,
				const dng_oriented_bounding_box &bBox)
	{
	
	

	const dng_point_real64 &a = aBox.fCenter;
	const dng_point_real64 &b = bBox.fCenter;

	const dng_point_real64 &a1 = aBox.fVec1;
	const dng_point_real64 &a2 = aBox.fVec2;

	const dng_point_real64 &b1 = bBox.fVec1;
	const dng_point_real64 &b2 = bBox.fVec2;

	

		{

		
		
		
		

		

		

		
		

		
		

		real64 a1_radius_a = Dot (a1, a1);

		real64 a1_radius_b = (Abs_real64 (Dot (b1, a1)) +
							  Abs_real64 (Dot (b2, a1)));

		real64 a1_dist = Abs_real64 (Dot (b - a, a1));
		
		if (a1_dist > a1_radius_a + a1_radius_b)
			{
			return false;
			}

		}

	

		{

		

		real64 a2_radius_a = Dot (a2, a2);

		real64 a2_radius_b = (Abs_real64 (Dot (b1, a2)) +
							  Abs_real64 (Dot (b2, a2)));

		real64 a2_dist = Abs_real64 (Dot (b - a, a2));

		if (a2_dist > a2_radius_a + a2_radius_b)
			{
			return false;
			}

		}

	

		{

		

		real64 b1_radius_b = Dot (b1, b1);

		real64 b1_radius_a = (Abs_real64 (Dot (a1, b1)) +
							  Abs_real64 (Dot (a2, b1)));

		real64 b1_dist = Abs_real64 (Dot (b - a, b1));

		if (b1_dist > b1_radius_a + b1_radius_b)
			{
			return false;
			}

		}

	

		{

		

		real64 b2_radius_b = Dot (b2, b2);

		real64 b2_radius_a = (Abs_real64 (Dot (a1, b2)) +
							  Abs_real64 (Dot (a2, b2)));

		real64 b2_dist = Abs_real64 (Dot (b - a, b2));

		if (b2_dist > b2_radius_a + b2_radius_b)
			{
			return false;
			}

		}

	

	return true;
	
	}

