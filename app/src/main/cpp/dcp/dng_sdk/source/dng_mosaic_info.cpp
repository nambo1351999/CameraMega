

#include "dng_mosaic_info.h"

#include "dng_area_task.h"
#include "dng_assertions.h"
#include "dng_bottlenecks.h"
#include "dng_exceptions.h"
#include "dng_filter_task.h"
#include "dng_host.h"
#include "dng_ifd.h"
#include "dng_image.h"
#include "dng_info.h"
#include "dng_negative.h"
#include "dng_pixel_buffer.h"
#include "dng_tag_types.h"
#include "dng_tag_values.h"
#include "dng_tile_iterator.h"
#include "dng_utils.h"

class dng_bilinear_kernel
	{
	
	public:
	
		enum
			{
			kMaxCount = 8
			};
	
		uint32 fCount;
	
		dng_point fDelta [kMaxCount];
		
		real32 fWeight32 [kMaxCount];
		uint16 fWeight16 [kMaxCount];
		
		int32 fOffset [kMaxCount];
		
	public:
	
		dng_bilinear_kernel ()
			:	fCount (0)
			{
			}
		
		void Add (const dng_point &delta,
				  real32 weight);
		
		void Finalize (const dng_point &scale,
					   uint32 patRow,
					   uint32 patCol,
					   int32 rowStep,
					   int32 colStep);
				  
	};
		

void dng_bilinear_kernel::Add (const dng_point &delta,
							   real32 weight)
	{
	
	
	
	if (weight <= 0.0f)
		{
		return;
		}
		
	
	
	
	for (uint32 j = 0; j < fCount; j++)
		{
		
		if (fDelta [j] == delta)
			{
			
			fWeight32 [j] += weight;
			
			return;
			
			}
			
		}
		
	
		
	DNG_ASSERT (fCount < kMaxCount, "Too many kernel entries");
	
	fDelta	  [fCount] = delta;
	fWeight32 [fCount] = weight;
	
	fCount++;

	}
				  

void dng_bilinear_kernel::Finalize (const dng_point &scale,
									uint32 patRow,
									uint32 patCol,
									int32 rowStep,
									int32 colStep)
	{
	
	uint32 j;
	
	
	
	for (j = 0; j < fCount; j++)
		{
		
		dng_point &delta = fDelta [j];
		
		if (scale.v == 2)
			{

			delta.v = (delta.v + (int32) (patRow & 1)) >> 1;

			}
			
		if (scale.h == 2)
			{
			
			delta.h = (delta.h + (int32) (patCol & 1)) >> 1;
			
			}
			
		}
		
	
	
	while (true)
		{
		
		bool didSwap = false;
		
		for (j = 1; j < fCount; j++)
			{
			
			dng_point &delta0 = fDelta [j - 1];
			dng_point &delta1 = fDelta [j	 ];
			
			if (delta0.v > delta1.v ||
					(delta0.v == delta1.v &&
					 delta0.h >	 delta1.h))
				{
				
				didSwap = true;

				dng_point tempDelta = delta0;
				
				delta0 = delta1;
				delta1 = tempDelta;
				
				real32 tempWeight = fWeight32 [j - 1];
				
				fWeight32 [j - 1] = fWeight32 [j];
				fWeight32 [j	] = tempWeight;
				
				}
			
			}
			
		if (!didSwap)
			{
			break;
			}
		
		}
	
	
	
	for (j = 0; j < fCount; j++)
		{
	
		fOffset [j] = rowStep * fDelta [j].v +
					  colStep * fDelta [j].h;
		
		}
		
	
	
	uint16 total   = 0;
	uint32 biggest = 0;
	
	for (j = 0; j < fCount; j++)
		{
		
		
		
		fWeight16 [j] = (uint16) Round_uint32 (fWeight32 [j] * 256.0);
		
		
		
		total += fWeight16 [j];
		
		
		
		if (fWeight16 [biggest] < fWeight16 [j])
			{
			
			biggest = j;
			
			}
		
		}
		
	
		
	fWeight16 [biggest] += (256 - total);
	
	
	
	
	for (j = 0; j < fCount; j++)
		{
		
		fWeight32 [j] = fWeight16 [j] * (1.0f / 256.0f);
		
		}
	
	}
				  

class dng_bilinear_pattern
	{
	
	public:
	
		enum
			{
			kMaxPattern = kMaxCFAPattern * 2
			};
			
		dng_point fScale;
	
		uint32 fPatRows;
		uint32 fPatCols;
	
		dng_bilinear_kernel fKernel [kMaxPattern]
									[kMaxPattern];
									
		uint32 fCounts [kMaxPattern]
					   [kMaxPattern];
					   
		int32 *fOffsets [kMaxPattern]
						[kMaxPattern];
						
		uint16 *fWeights16 [kMaxPattern]
						   [kMaxPattern];
						   
		real32 *fWeights32 [kMaxPattern]
						   [kMaxPattern];
						   
	public:
		
		dng_bilinear_pattern ()
			
			:	fScale ()
			,	fPatRows (0)
			,	fPatCols (0)
			
			{
			}
		
	private:
	
		DNG_ATTRIB_NO_SANITIZE("unsigned-integer-overflow")
		uint32 DeltaRow (uint32 row, int32 delta)
			{
			
			
			return (SafeUint32Add (row, fPatRows) + (uint32) delta) % fPatRows;
			}
			
		DNG_ATTRIB_NO_SANITIZE("unsigned-integer-overflow")
		uint32 DeltaCol (uint32 col, int32 delta)
			{
			
			
			return (SafeUint32Add (col, fPatCols) + (uint32) delta) % fPatCols;
			}
	
		real32 LinearWeight1 (int32 d1, int32 d2)
			{
			if (d1 == d2)
				return 1.0f;
			else
				return d2 / (real32) (d2 - d1);
			}
	
		real32 LinearWeight2 (int32 d1, int32 d2)
			{
			if (d1 == d2)
				return 0.0f;
			else
				return -d1 / (real32) (d2 - d1);
			}
			
	public:
	
		void Calculate (const dng_mosaic_info &info,
						uint32 dstPlane,
						int32 rowStep,
						int32 colStep);
	
	};

void dng_bilinear_pattern::Calculate (const dng_mosaic_info &info,
									  uint32 dstPlane,
									  int32 rowStep,
									  int32 colStep)
	{
	
	uint32 j;
	uint32 k;
	uint32 patRow;
	uint32 patCol;
	
	
	
	fScale = info.FullScale ();
	
	fPatRows = info.fCFAPatternSize.v * fScale.v;
	fPatCols = info.fCFAPatternSize.h * fScale.h;
	
	
	
	dng_point tempScale (1, 1);
	
	if (info.fCFALayout >= 6)
		{
		
		tempScale = dng_point (2, 2);
		
		fPatRows *= tempScale.v;
		fPatCols *= tempScale.h;

		}
		
	
	
	bool map [kMaxPattern]
			 [kMaxPattern];
			  
	uint8 planeColor = info.fCFAPlaneColor [dstPlane];
	
	switch (info.fCFALayout)
		{
		
		case 1:		
			{
			
			for (j = 0; j < fPatRows; j++)
				{
				
				for (k = 0; k < fPatCols; k++)
					{
					
					map [j] [k] = (info.fCFAPattern [j] [k] == planeColor);
					
					}
					
				}
			
			break;
			
			}
			
		
		
		
			
		case 2:		
			{
			
			for (j = 0; j < fPatRows; j++)
				{
				
				for (k = 0; k < fPatCols; k++)
					{
					
					if ((j & 1) != (k & 1))
						{
						
						map [j] [k] = false;
						
						}
						
					else
						{
						
						map [j] [k] = (info.fCFAPattern [j >> 1] [k] == planeColor);
						
						}
						
					}
					
				}
				
			break;
			
			}
					
		case 3:		
			{
			
			for (j = 0; j < fPatRows; j++)
				{
				
				for (k = 0; k < fPatCols; k++)
					{
					
					if ((j & 1) == (k & 1))
						{
						
						map [j] [k] = false;
						
						}
						
					else
						{
						
						map [j] [k] = (info.fCFAPattern [j >> 1] [k] == planeColor);
						
						}
						
					}
					
				}
				
			break;
			
			}
					
		case 4:		
			{
			
			for (j = 0; j < fPatRows; j++)
				{
				
				for (k = 0; k < fPatCols; k++)
					{
					
					if ((j & 1) != (k & 1))
						{
						
						map [j] [k] = false;
						
						}
						
					else
						{
						
						map [j] [k] = (info.fCFAPattern [j] [k >> 1] == planeColor);
						
						}
						
					}
					
				}
				
			break;
			
			}
					
		case 5:		
			{
			
			for (j = 0; j < fPatRows; j++)
				{
				
				for (k = 0; k < fPatCols; k++)
					{
					
					if ((j & 1) == (k & 1))
						{
						
						map [j] [k] = false;
						
						}
						
					else
						{
						
						map [j] [k] = (info.fCFAPattern [j] [k >> 1] == planeColor);
						
						}
						
					}
					
				}
				
			break;
			
			}
			
		case 6:		
		case 7:		
		case 8:		
		case 9:		
			{
			
			uint32 eRow = (info.fCFALayout == 6 ||
						   info.fCFALayout == 7) ? 1 : 3;
						   
			uint32 eCol = (info.fCFALayout == 6 ||
						   info.fCFALayout == 8) ? 1 : 3;
			
			for (j = 0; j < fPatRows; j++)
				{
				
				for (k = 0; k < fPatCols; k++)
					{
					
					uint32 jj = j & 3;
					uint32 kk = k & 3;
					
					if ((jj != 0 && jj != eRow) ||
						(kk != 0 && kk != eCol))
						{
						
						map [j] [k] = false;
						
						}
						
					else
						{
						
						map [j] [k] = (info.fCFAPattern [((j >> 1) & ~1) + Min_uint32 (jj, 1)]
														[((k >> 1) & ~1) + Min_uint32 (kk, 1)] == planeColor);
						
						}
						
					}
					
				}
			
			break;
			
			}
					
		default:
			ThrowProgramError ();
		
		}
		
	
	
	bool mapH [kMaxPattern];
	bool mapV [kMaxPattern];
	
	for (j = 0; j < kMaxPattern; j++)
		{
		
		mapH [j] = false;
		mapV [j] = false;
		
		}
		
	for (j = 0; j < fPatRows; j++)
		{
		
		for (k = 0; k < fPatCols; k++)
			{
			
			if (map [j] [k])
				{
				
				mapV [j] = true;
				mapH [k] = true;
				
				}
				
			}
			
		}
	
	
	
	for (patRow = 0; patRow < fPatRows; patRow += tempScale.v)
		{
		
		for (patCol = 0; patCol < fPatCols; patCol += tempScale.h)
			{
			
			dng_bilinear_kernel &kernel = fKernel [patRow] [patCol];
			
			
			
			if (map [patRow] [patCol])
				{
				
				kernel.Add (dng_point (0, 0), 1.0f);
				
				continue;
				
				}
				
			
			
			uint32 n = DeltaRow (patRow, -1);
			uint32 s = DeltaRow (patRow,  1);
			uint32 w = DeltaCol (patCol, -1);
			uint32 e = DeltaCol (patCol,  1);
			
			bool mapNW = map [n] [w];
			bool mapN  = map [n] [patCol];
			bool mapNE = map [n] [e];
			
			bool mapW = map [patRow] [w];
			bool mapE = map [patRow] [e];
			
			bool mapSW = map [s] [w];
			bool mapS  = map [s] [patCol];
			bool mapSE = map [s] [e];
			
			
			
			if (mapN && mapS && mapW && mapW)
				{
				
				kernel.Add (dng_point (-1,	0), 0.25f);
				kernel.Add (dng_point ( 0, -1), 0.25f);
				kernel.Add (dng_point ( 0,	1), 0.25f);
				kernel.Add (dng_point ( 1,	0), 0.25f);
				
				continue;
				
				}
			
			
			
			if (mapN && mapS)
				{
				
				kernel.Add (dng_point (-1,	0), 0.5f);
				kernel.Add (dng_point ( 1,	0), 0.5f);
				
				continue;
				
				}
			
			
			
			if (mapW && mapE)
				{
				
				kernel.Add (dng_point ( 0, -1), 0.5f);
				kernel.Add (dng_point ( 0,	1), 0.5f);
				
				continue;
				
				}
				
			
			
			if (mapN && mapSW && mapSE)
				{
				
				kernel.Add (dng_point (-1,	0), 0.50f);
				kernel.Add (dng_point ( 1, -1), 0.25f);
				kernel.Add (dng_point ( 1,	1), 0.25f);
				
				continue;
				
				}
			
			
			
			if (mapS && mapNW && mapNE)
				{
				
				kernel.Add (dng_point (-1, -1), 0.25f);
				kernel.Add (dng_point (-1,	1), 0.25f);
				kernel.Add (dng_point ( 1,	0), 0.50f);
				
				continue;
				
				}
			
			
			
			if (mapW && mapNE && mapSE)
				{
				
				kernel.Add (dng_point (-1,	1), 0.25f);
				kernel.Add (dng_point ( 0, -1), 0.50f);
				kernel.Add (dng_point ( 1,	1), 0.25f);
				
				continue;
				
				}
			
			
			
			if (mapE && mapNW && mapSW)
				{
				
				kernel.Add (dng_point (-1, -1), 0.25f);
				kernel.Add (dng_point ( 0,	1), 0.50f);
				kernel.Add (dng_point ( 1, -1), 0.25f);
				
				continue;
				
				}
				
			
			
			if (mapNW && mapNE && mapSE && mapSW)
				{
				
				kernel.Add (dng_point (-1, -1), 0.25f);
				kernel.Add (dng_point (-1,	1), 0.25f);
				kernel.Add (dng_point ( 1, -1), 0.25f);
				kernel.Add (dng_point ( 1,	1), 0.25f);
				
				continue;
				
				}
			
			
			
			if (mapNW && mapSE)
				{
				
				kernel.Add (dng_point (-1, -1), 0.50f);
				kernel.Add (dng_point ( 1,	1), 0.50f);
				
				continue;
				
				}
				
			
			
			if (mapNE && mapSW)
				{
				
				kernel.Add (dng_point (-1,	1), 0.50f);
				kernel.Add (dng_point ( 1, -1), 0.50f);
				
				continue;
				
				}
				
			
			
			int32 dv1 = 0;
			int32 dv2 = 0;
			
			while (!mapV [DeltaRow (patRow, dv1)])
				{
				dv1--;
				}
				
			while (!mapV [DeltaRow (patRow, dv2)])
				{
				dv2++;
				}
				
			real32 w1 = LinearWeight1 (dv1, dv2) * 0.5f;
			real32 w2 = LinearWeight2 (dv1, dv2) * 0.5f;
				
			int32 v1 = DeltaRow (patRow, dv1);
			int32 v2 = DeltaRow (patRow, dv2);
				
			int32 dh1 = 0;
			int32 dh2 = 0;
			
			while (!map [v1] [DeltaCol (patCol, dh1)])
				{
				dh1--;
				}
				
			while (!map [v1] [DeltaCol (patCol, dh2)])
				{
				dh2++;
				}
				
			kernel.Add (dng_point (dv1, dh1),
						LinearWeight1 (dh1, dh2) * w1);
						
			kernel.Add (dng_point (dv1, dh2),
						LinearWeight2 (dh1, dh2) * w1);
			
			dh1 = 0;
			dh2 = 0;
			
			while (!map [v2] [DeltaCol (patCol, dh1)])
				{
				dh1--;
				}
				
			while (!map [v2] [DeltaCol (patCol, dh2)])
				{
				dh2++;
				}
				
			kernel.Add (dng_point (dv2, dh1),
						LinearWeight1 (dh1, dh2) * w2);
						
			kernel.Add (dng_point (dv2, dh2),
						LinearWeight2 (dh1, dh2) * w2);
						
			dh1 = 0;
			dh2 = 0;
			
			while (!mapH [DeltaCol (patCol, dh1)])
				{
				dh1--;
				}
				
			while (!mapH [DeltaCol (patCol, dh2)])
				{
				dh2++;
				}
				
			w1 = LinearWeight1 (dh1, dh2) * 0.5f;
			w2 = LinearWeight2 (dh1, dh2) * 0.5f;
				
			int32 h1 = DeltaCol (patCol, dh1);
			int32 h2 = DeltaCol (patCol, dh2);
				
			dv1 = 0;
			dv2 = 0;
			
			while (!map [DeltaRow (patRow, dv1)] [h1])
				{
				dv1--;
				}
				
			while (!map [DeltaRow (patRow, dv2)] [h1])
				{
				dv2++;
				}
				
			kernel.Add (dng_point (dv1, dh1),
						LinearWeight1 (dv1, dv2) * w1);
						
			kernel.Add (dng_point (dv2, dh1),
						LinearWeight2 (dv1, dv2) * w1);
				
			dv1 = 0;
			dv2 = 0;
			
			while (!map [DeltaRow (patRow, dv1)] [h2])
				{
				dv1--;
				}
				
			while (!map [DeltaRow (patRow, dv2)] [h2])
				{
				dv2++;
				}
				
			kernel.Add (dng_point (dv1, dh2),
						LinearWeight1 (dv1, dv2) * w2);
						
			kernel.Add (dng_point (dv2, dh2),
						LinearWeight2 (dv1, dv2) * w2);
						
			}
			
		}
		
	
	
	if (tempScale == dng_point (2, 2))
		{
		
		fPatRows /= tempScale.v;
		fPatCols /= tempScale.h;
		
		for (patRow = 0; patRow < fPatRows; patRow++)
			{
			
			for (patCol = 0; patCol < fPatCols; patCol++)
				{
				
				int32 patRow2 = patRow << 1;
				int32 patCol2 = patCol << 1;
				
				dng_bilinear_kernel &kernel = fKernel [patRow2] [patCol2];
				
				for (j = 0; j < kernel.fCount; j++)
					{
					
					int32 x = patRow2 + kernel.fDelta [j].v;
					
					if ((x & 3) != 0)
						{
						x = (x & ~3) + 2;
						}
						
					kernel.fDelta [j].v = ((x - patRow2) >> 1);
					
					x = patCol2 + kernel.fDelta [j].h;
					
					if ((x & 3) != 0)
						{
						x = (x & ~3) + 2;
						}
						
					kernel.fDelta [j].h = ((x - patCol2) >> 1);
					
					}

				kernel.Finalize (fScale,
								 patRow,
								 patCol,
								 rowStep,
								 colStep);
										 
				fCounts	   [patRow] [patCol] = kernel.fCount;
				fOffsets   [patRow] [patCol] = kernel.fOffset;
				fWeights16 [patRow] [patCol] = kernel.fWeight16;
				fWeights32 [patRow] [patCol] = kernel.fWeight32;
			
				}
				
			}
			
		}
		
	
	
	else
		{
		
		for (patRow = 0; patRow < fPatRows; patRow++)
			{
			
			for (patCol = 0; patCol < fPatCols; patCol++)
				{
				
				dng_bilinear_kernel &kernel = fKernel [patRow] [patCol];
				
				kernel.Finalize (fScale,
								 patRow,
								 patCol,
								 rowStep,
								 colStep);
										 
				fCounts	   [patRow] [patCol] = kernel.fCount;
				fOffsets   [patRow] [patCol] = kernel.fOffset;
				fWeights16 [patRow] [patCol] = kernel.fWeight16;
				fWeights32 [patRow] [patCol] = kernel.fWeight32;
				
				}
				
			}
			
		}
		
	}
	

class dng_bilinear_interpolator
	{
	
	private:
	
		dng_bilinear_pattern fPattern [kMaxColorPlanes];
		
	public:
	
		dng_bilinear_interpolator (const dng_mosaic_info &info,
								   int32 rowStep,
								   int32 colStep);
		
		void Interpolate (dng_pixel_buffer &srcBuffer,
						  dng_pixel_buffer &dstBuffer);
	
	};

dng_bilinear_interpolator::dng_bilinear_interpolator (const dng_mosaic_info &info,
													  int32 rowStep,
													  int32 colStep)
	{
	
	for (uint32 dstPlane = 0; dstPlane < info.fColorPlanes; dstPlane++)
		{
		
		fPattern [dstPlane] . Calculate (info, 
										 dstPlane,
										 rowStep,
										 colStep);
				
		}
	
	}

void dng_bilinear_interpolator::Interpolate (dng_pixel_buffer &srcBuffer,
											 dng_pixel_buffer &dstBuffer)
	{
	
	uint32 patCols = fPattern [0] . fPatCols;
	uint32 patRows = fPattern [0] . fPatRows;
	
	dng_point scale = fPattern [0] . fScale;
	
	uint32 sRowShift = scale.v - 1;
	uint32 sColShift = scale.h - 1;
	
	int32 dstCol = dstBuffer.fArea.l;
	
	int32 srcCol = dstCol >> sColShift;
	
	uint32 patPhase = dstCol % patCols;
	
	for (int32 dstRow = dstBuffer.fArea.t; 
		 dstRow < dstBuffer.fArea.b;
		 dstRow++)
		{
		
		int32 srcRow = dstRow >> sRowShift;
		
		uint32 patRow = dstRow % patRows;
		
		for (uint32 dstPlane = 0;
			 dstPlane < dstBuffer.fPlanes;
			 dstPlane++)
			{
			
			const void *sPtr = srcBuffer.ConstPixel (srcRow,
													  srcCol,
													  srcBuffer.fPlane);
												  
			void *dPtr = dstBuffer.DirtyPixel (dstRow,
											   dstCol,
											   dstPlane);
										  
			if (dstBuffer.fPixelType == ttShort)
				{
				
				DoBilinearRow16 ((const uint16 *) sPtr,
								 (uint16 *) dPtr,
								 dstBuffer.fArea.W (),
								 patPhase,
								 patCols,
								 fPattern [dstPlane].fCounts	[patRow],
								 fPattern [dstPlane].fOffsets	[patRow],
								 fPattern [dstPlane].fWeights16 [patRow],
								 sColShift);
				
				}
				
			else
				{
				
				DoBilinearRow32 ((const real32 *) sPtr,
								 (real32 *) dPtr,
								 dstBuffer.fArea.W (),
								 patPhase,
								 patCols,
								 fPattern [dstPlane].fCounts	[patRow],
								 fPattern [dstPlane].fOffsets	[patRow],
								 fPattern [dstPlane].fWeights32 [patRow],
								 sColShift);
				
				}
											
			}
		
		}
	
	}
	

class dng_fast_interpolator: public dng_filter_task
	{
	
	protected:
	
		const dng_mosaic_info &fInfo;
	
		dng_point fDownScale;
		
		uint32 fFilterColor [kMaxCFAPattern] [kMaxCFAPattern];
		
	public:
	
		dng_fast_interpolator (const dng_mosaic_info &info,
							   const dng_image &srcImage,
							   dng_image &dstImage,
							   const dng_point &downScale,
							   uint32 srcPlane);
							   
		virtual dng_rect SrcArea (const dng_rect &dstArea);
			
		virtual void ProcessArea (uint32 threadIndex,
								  dng_pixel_buffer &srcBuffer,
								  dng_pixel_buffer &dstBuffer);
		
	};

dng_fast_interpolator::dng_fast_interpolator (const dng_mosaic_info &info,
											  const dng_image &srcImage,
											  dng_image &dstImage,
											  const dng_point &downScale,
											  uint32 srcPlane)
	
	:	dng_filter_task ("dng_fast_interpolator",
						 srcImage,
						 dstImage)
	
	,	fInfo		(info	  )
	,	fDownScale	(downScale)
	
	{
	
	fSrcPlane  = srcPlane;
	fSrcPlanes = 1;
	
	fSrcPixelType = ttShort;
	fDstPixelType = ttShort;
	
	fSrcRepeat = fInfo.fCFAPatternSize;
	
	fUnitCell = fInfo.fCFAPatternSize;
	
	fMaxTileSize = dng_point (256 / fDownScale.v,
							  256 / fDownScale.h);
							  
	fMaxTileSize.h = Max_int32 (fMaxTileSize.h, fUnitCell.h);
	fMaxTileSize.v = Max_int32 (fMaxTileSize.v, fUnitCell.v);
							  
	
	
		{
		
		for (int32 r = 0; r < fInfo.fCFAPatternSize.v; r++)
			{
			
			for (int32 c = 0; c < fInfo.fCFAPatternSize.h; c++)
				{
				
				uint8 key = fInfo.fCFAPattern [r] [c];
				
				for (uint32 index = 0; index < fInfo.fColorPlanes; index++)
					{
					
					if (key == fInfo.fCFAPlaneColor [index])
						{
						
						fFilterColor [r] [c] = index;
						
						break;
						
						}
						
					}
				
				}
				
			}
				
		}

	}

dng_rect dng_fast_interpolator::SrcArea (const dng_rect &dstArea)
	{
	
	return dng_rect (dstArea.t * fDownScale.v,
					 dstArea.l * fDownScale.h,
					 dstArea.b * fDownScale.v,
					 dstArea.r * fDownScale.h);
	
	}
			

void dng_fast_interpolator::ProcessArea (uint32 ,
										 dng_pixel_buffer &srcBuffer,
										 dng_pixel_buffer &dstBuffer)
	{
	
	dng_rect srcArea = srcBuffer.fArea;
	dng_rect dstArea = dstBuffer.fArea;
					  
	
	
	int32 srcRow = srcArea.t;
	
	uint32 srcRowPhase1 = 0;
	uint32 srcRowPhase2 = 0;
	
	uint32 patRows = fInfo.fCFAPatternSize.v;
	uint32 patCols = fInfo.fCFAPatternSize.h;
	
	uint32 cellRows = fDownScale.v;
	uint32 cellCols = fDownScale.h;
	
	uint32 plane;
	uint32 planes = fInfo.fColorPlanes;
	
	int32 dstPlaneStep = dstBuffer.fPlaneStep;
	
	uint32 total [kMaxColorPlanes];
	uint32 count [kMaxColorPlanes];
	
	for (plane = 0; plane < planes; plane++)
		{
		total [plane] = 0;
		count [plane] = 0;
		}
			
	for (int32 dstRow = dstArea.t; dstRow < dstArea.b; dstRow++)
		{
		
		const uint16 *sPtr = srcBuffer.ConstPixel_uint16 (srcRow,
														  srcArea.l,
														  fSrcPlane);
																  
		uint16 *dPtr = dstBuffer.DirtyPixel_uint16 (dstRow,
													dstArea.l,
													0);
											 
		uint32 srcColPhase1 = 0;
		uint32 srcColPhase2 = 0;
		
		for (int32 dstCol = dstArea.l; dstCol < dstArea.r; dstCol++)
			{
			
			const uint16 *ssPtr = sPtr;
			
			srcRowPhase2 = srcRowPhase1;
			
			for (uint32 cellRow = 0; cellRow < cellRows; cellRow++)
				{
				
				const uint32 *filterRow = fFilterColor [srcRowPhase2];
			
				if (++srcRowPhase2 == patRows)
					{
					srcRowPhase2 = 0;
					}
					
				srcColPhase2 = srcColPhase1;
				
				for (uint32 cellCol = 0; cellCol < cellCols; cellCol++)
					{
				
					uint32 color = filterRow [srcColPhase2];
					
					if (++srcColPhase2 == patCols)
						{
						srcColPhase2 = 0;
						}
					
					total [color] += (uint32) ssPtr [cellCol];
					count [color] ++;
					
					}
					
				ssPtr += srcBuffer.fRowStep;
				
				}
				
			for (plane = 0; plane < planes; plane++)
				{
				
				uint32 t = total [plane];
				uint32 c = Max_uint32 (count [plane], 1);
				
				dPtr [plane * dstPlaneStep] = (uint16) ((t + (c >> 1)) / c);
				
				total [plane] = 0;
				count [plane] = 0;
				
				}
				
			srcColPhase1 = srcColPhase2;
				
			sPtr += cellCols;
				
			dPtr ++;

			}
			
		srcRowPhase1 = srcRowPhase2;
			
		srcRow += cellRows;
		
		}
				   
	}
	

dng_mosaic_info::dng_mosaic_info ()

	:	fCFAPatternSize	 ()
	,	fColorPlanes	 (0)
	,	fCFALayout		 (1)
	,	fBayerGreenSplit (0)
	,	fSrcSize		 ()
	,	fCroppedSize	 ()
	,	fAspectRatio	 (1.0)
	
	{
	
	}

dng_mosaic_info::~dng_mosaic_info ()
	{
	
	}
		

void dng_mosaic_info::Parse (dng_host & ,
							 dng_stream & ,
							 dng_info &info)
	{
	
	
	
	dng_ifd &rawIFD = *info.fIFD [info.fMainIndex];
	
	
	
	if (rawIFD.fPhotometricInterpretation != piCFA)
		{
		return;
		}
	
	
	
	fCFAPatternSize.v = rawIFD.fCFARepeatPatternRows;
	fCFAPatternSize.h = rawIFD.fCFARepeatPatternCols;

	DNG_REQUIRE (fCFAPatternSize.v >= 1 && fCFAPatternSize.v <= (int32) kMaxCFAPattern,
				 "Invalid fCFAPatternSize.v");
	
	DNG_REQUIRE (fCFAPatternSize.h >= 1 && fCFAPatternSize.h <= (int32) kMaxCFAPattern,
				 "Invalid fCFAPatternSize.h");
	
	for (int32 j = 0; j < fCFAPatternSize.v; j++)
		{
		for (int32 k = 0; k < fCFAPatternSize.h; k++)
			{
			fCFAPattern [j] [k] = rawIFD.fCFAPattern [j] [k];
			}
		}
		
	
	
	fColorPlanes = info.fShared->fCameraProfile.fColorPlanes;
	
	for (uint32 n = 0; n < fColorPlanes; n++)
		{
		fCFAPlaneColor [n] = rawIFD.fCFAPlaneColor [n];
		}
		
	
	
	fCFALayout = rawIFD.fCFALayout;
	
	
	
	fBayerGreenSplit = rawIFD.fBayerGreenSplit;
	
	}
		

void dng_mosaic_info::PostParse (dng_host & ,
								 dng_negative &negative)
	{
	
	
	
	fSrcSize = negative.Stage2Image ()->Size ();
	
	
	
	fCroppedSize.v = Round_int32 (negative.DefaultCropSizeV ().As_real64 ());
	fCroppedSize.h = Round_int32 (negative.DefaultCropSizeH ().As_real64 ());
	
	
	
	fAspectRatio = negative.DefaultScaleH ().As_real64 () /
				   negative.DefaultScaleV ().As_real64 ();
	
	}
							

bool dng_mosaic_info::SetFourColorBayer ()
	{
	
	if (fCFAPatternSize != dng_point (2, 2))
		{
		return false;
		}
		
	if (fColorPlanes != 3)
		{
		return false;
		}
		
	uint8 color0 = fCFAPlaneColor [0];
	uint8 color1 = fCFAPlaneColor [1];
	uint8 color2 = fCFAPlaneColor [2];
	
	
	
	if ((fCFAPattern [0] [0] == color1 && fCFAPattern [1] [1] == color1) ||
		(fCFAPattern [0] [1] == color1 && fCFAPattern [1] [0] == color1))
		{
		
		
		
		
		
		uint8 color3 = 0;
		
		while (color3 == color0 ||
			   color3 == color1 ||
			   color3 == color2)
			{
			color3++;
			}
			
		
		
		fColorPlanes = 4;
		
		fCFAPlaneColor [3] = color3;
		
		
		
		if (fCFAPattern [0] [0] == color0)
			{
			fCFAPattern [1] [0] = color3;
			}
			
		else if (fCFAPattern [0] [1] == color0)
			{
			fCFAPattern [1] [1] = color3;
			}
			
		else if (fCFAPattern [1] [0] == color0)
			{
			fCFAPattern [0] [0] = color3;
			}
			
		else
			{
			fCFAPattern [0] [1] = color3;
			}
			
		return true;

		}
	
	return false;
	
	}
							

dng_point dng_mosaic_info::FullScale () const
	{
	
	switch (fCFALayout)
		{
		
		
		
		
		case 2:
		case 3:
			return dng_point (2, 1);
			
		
		
			
		case 4:
		case 5:
			return dng_point (1, 2);
			
		
		
		default:
			break;
		
		}
		
	return dng_point (1, 1);
	
	}
	

bool dng_mosaic_info::IsSafeDownScale (const dng_point &downScale) const
	{

	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	

	if (fCFAPatternSize.v < 1 ||
		fCFAPatternSize.v > (int32) kMaxCFAPattern ||
		fCFAPatternSize.h < 1 ||
		fCFAPatternSize.h > (int32) kMaxCFAPattern ||
		fColorPlanes > kMaxColorPlanes)
		{

		return false;

		}

	if (downScale.v >= fCFAPatternSize.v &&
		downScale.h >= fCFAPatternSize.h)
		{

		return true;

		}
		
	dng_point test;
	
	test.v = Min_int32 (downScale.v, fCFAPatternSize.v);
	test.h = Min_int32 (downScale.h, fCFAPatternSize.h);
		
	for (int32 phaseV = 0; phaseV < fCFAPatternSize.v; phaseV++)
		{
		
		for (int32 phaseH = 0; phaseH < fCFAPatternSize.h; phaseH++)
			{
			
			uint32 plane;
			
			bool contains [kMaxColorPlanes];
			
			for (plane = 0; plane < fColorPlanes; plane++)
				{
				
				contains [plane] = false;
				
				}
			
			for (int32 srcRow = 0; srcRow < test.v; srcRow++)
				{
				
				for (int32 srcCol = 0; srcCol < test.h; srcCol++)
					{
					
					uint8 srcKey = fCFAPattern [(srcRow + phaseV) % fCFAPatternSize.v]
											   [(srcCol + phaseH) % fCFAPatternSize.h];
											   
					for (plane = 0; plane < fColorPlanes; plane++)
						{
						
						if (srcKey == fCFAPlaneColor [plane])
							{
							
							contains [plane] = true;
							
							}
							
						}
					
					
					}
					
				}
				
			for (plane = 0; plane < fColorPlanes; plane++)
				{
				
				if (!contains [plane])
					{
					
					return false;
					
					}
					
				}
			
			}
		
		}
		
	return true;
	
	}
	

uint32 dng_mosaic_info::SizeForDownScale (const dng_point &downScale) const
	{
	
	uint32 sizeV = Max_uint32 (1, (fCroppedSize.v + (downScale.v >> 1)) / downScale.v);
	uint32 sizeH = Max_uint32 (1, (fCroppedSize.h + (downScale.h >> 1)) / downScale.h);
	
	return Max_int32 (sizeV, sizeH);
	
	}
	

bool dng_mosaic_info::ValidSizeDownScale (const dng_point &downScale,
										  uint32 minSize) const
	{
	
	const int32 kMaxDownScale = 64;
	
	if (downScale.h > kMaxDownScale ||
		downScale.v > kMaxDownScale)
		{
		
		return false;
		
		}
		
	return SizeForDownScale (downScale) >= minSize;
			
	}

dng_point dng_mosaic_info::DownScale (uint32 minSize,
									  uint32 prefSize,
									  real64 cropFactor) const
	{
	
	dng_point bestScale (1, 1);
	
	if (prefSize && IsColorFilterArray ())
		{
		
		
		
		minSize	 = Round_uint32 (minSize  / cropFactor);
		prefSize = Round_uint32 (prefSize / cropFactor);
		
		prefSize = Max_uint32 (prefSize, minSize);
		
		
		
		int32 bestSize = SizeForDownScale (bestScale);
		
		
		
		dng_point squareCell (1, 1);
		
		if (fAspectRatio < 1.0 / 1.8)
			{
			
			squareCell.h = Min_int32 (4, Round_int32 (1.0 / fAspectRatio));
			
			}
		
		if (fAspectRatio > 1.8)
			{
			
			squareCell.v = Min_int32 (4, Round_int32 (fAspectRatio));
			
			}
	
		
		
		dng_point testScale = squareCell;
		
		while (!IsSafeDownScale (testScale))
			{
			
			testScale.v += squareCell.v;
			testScale.h += squareCell.h;
			
			}
		
		
		
		if (!ValidSizeDownScale (testScale, minSize))
			{
			
			
			
			return bestScale;
			
			}
			
		
		
		int32 testSize = SizeForDownScale (testScale);
		
		if (Abs_int32 (testSize - (int32) prefSize) <=
			Abs_int32 (bestSize - (int32) prefSize))
			{
			bestScale = testScale;
			bestSize  = testSize;
			}
			
		else
			{
			return bestScale;
			}
		
		
		
		while (true)
			{
			
			testScale.v += squareCell.v;
			testScale.h += squareCell.h;
			
			if (IsSafeDownScale (testScale))
				{
				
				if (!ValidSizeDownScale (testScale, minSize))
					{
					return bestScale;
					}
				
				
				
				testSize = SizeForDownScale (testScale);
				
				if (Abs_int32 (testSize - (int32) prefSize) <=
					Abs_int32 (bestSize - (int32) prefSize))
					{
					bestScale = testScale;
					bestSize  = testSize;
					}
					
				else
					{
					return bestScale;
					}
						
				}
				
			}
		
		}
	
	return bestScale;
	
	}

dng_point dng_mosaic_info::DstSize (const dng_point &downScale) const
	{
	
	if (downScale == dng_point (1, 1))
		{
	
		dng_point scale = FullScale ();
		
		return dng_point (fSrcSize.v * scale.v,
						  fSrcSize.h * scale.h);
						  
		}
		
	const int32 kMaxDownScale = 64;
	
	if (downScale.h > kMaxDownScale ||
		downScale.v > kMaxDownScale)
		{
		
		return dng_point (0, 0);
		
		}
		
	dng_point size;
	
	size.v = Max_int32 (1, (fSrcSize.v + (downScale.v >> 1)) / downScale.v);
	size.h = Max_int32 (1, (fSrcSize.h + (downScale.h >> 1)) / downScale.h);
	
	return size;
			
	}

void dng_mosaic_info::InterpolateGeneric (dng_host &host,
										  dng_negative & ,
										  const dng_image &srcImage,
										  dng_image &dstImage,
										  uint32 srcPlane) const
	{
	
	
	
	dng_point scale = FullScale ();
	
	uint32 srcShiftV = scale.v - 1;
	uint32 srcShiftH = scale.h - 1;
	
	
	
	const uint32 kMaxDstTileRows = 128;
	const uint32 kMaxDstTileCols = 128;
	
	dng_point dstTileSize = dstImage.RepeatingTile ().Size ();
	
	dstTileSize.v = Min_int32 (dstTileSize.v, kMaxDstTileRows);
	dstTileSize.h = Min_int32 (dstTileSize.h, kMaxDstTileCols);
	
	dng_point srcTileSize = dstTileSize;
	
	srcTileSize.v >>= srcShiftV;
	srcTileSize.h >>= srcShiftH;
	
	srcTileSize.v += fCFAPatternSize.v * 2;
	srcTileSize.h += fCFAPatternSize.h * 2;
	
	
	
	dng_pixel_buffer srcBuffer (dng_rect (srcTileSize), 
								srcPlane, 
								1,
								srcImage.PixelType (), 
								pcInterleaved, 
								NULL);
	
	uint32 srcBufferSize = ComputeBufferSize (srcBuffer.fPixelType,
											  srcTileSize, 
											  srcBuffer.fPlanes,
											  padNone);
	
	AutoPtr<dng_memory_block> srcData (host.Allocate (srcBufferSize));
	
	srcBuffer.fData = srcData->Buffer ();
	
	
	
	dng_pixel_buffer dstBuffer (dng_rect (dstTileSize), 
								0, 
								fColorPlanes,
								dstImage.PixelType (), 
								pcRowInterleaved, 
								NULL);
	
	uint32 dstBufferSize = ComputeBufferSize (dstBuffer.fPixelType,
											  dstTileSize, 
											  dstBuffer.fPlanes,
											  padNone);
	
	AutoPtr<dng_memory_block> dstData (host.Allocate (dstBufferSize));
	
	dstBuffer.fData = dstData->Buffer ();
	
	

	AutoPtr<dng_bilinear_interpolator> interpolator (new dng_bilinear_interpolator (*this,
																					srcBuffer.fRowStep,
																					srcBuffer.fColStep));

	
	
	dng_rect dstArea;
	
	dng_tile_iterator iter1 (dstImage, dstImage.Bounds ());
	
	while (iter1.GetOneTile (dstArea))
		{
		
		
		
		dng_rect dstTile;
		
		dng_tile_iterator iter2 (dstTileSize, dstArea);
		
		while (iter2.GetOneTile (dstTile))
			{
			
			host.SniffForAbort ();
			
			
			
			dng_rect srcTile (dstTile);
			
			srcTile.t >>= srcShiftV;
			srcTile.b >>= srcShiftV;
			
			srcTile.l >>= srcShiftH;
			srcTile.r >>= srcShiftH;
			
			srcTile.t -= fCFAPatternSize.v;
			srcTile.b += fCFAPatternSize.v;
			
			srcTile.l -= fCFAPatternSize.h;
			srcTile.r += fCFAPatternSize.h;
			
			srcBuffer.fArea = srcTile;
			dstBuffer.fArea = dstTile;
			
			
			
			srcImage.Get (srcBuffer,
						  dng_image::edge_repeat,
						  fCFAPatternSize.v,
						  fCFAPatternSize.h);
						  
			
			
			interpolator->Interpolate (srcBuffer,
									   dstBuffer);
									  
			
			
			dstImage.Put (dstBuffer);
							  
			}
			
		}
		
	}

void dng_mosaic_info::InterpolateFast (dng_host &host,
									   dng_negative & ,
									   const dng_image &srcImage,
									   dng_image &dstImage,
									   const dng_point &downScale,
									   uint32 srcPlane) const
	{

	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	

	if (fCFAPatternSize.v < 1 ||
		fCFAPatternSize.v > (int32) kMaxCFAPattern ||
		fCFAPatternSize.h < 1 ||
		fCFAPatternSize.h > (int32) kMaxCFAPattern ||
		fColorPlanes > kMaxColorPlanes)
		{

		ThrowProgramError ("Invalid CFA pattern for fast interpolation");

		}

	

	dng_fast_interpolator interpolator (*this,
										srcImage,
										dstImage,
										downScale,
										srcPlane);
	
	
	
	dng_rect bounds = dstImage.Bounds ();
	
	
	
	host.PerformAreaTask (interpolator,
						  bounds);
	
	}
	

void dng_mosaic_info::Interpolate (dng_host &host,
								   dng_negative &negative,
								   const dng_image &srcImage,
								   dng_image &dstImage,
								   const dng_point &downScale,
								   uint32 srcPlane,
								   dng_matrix *scaleTransforms) const
	{
	
	if (scaleTransforms && downScale != dng_point (1, 1))
		{
		
		for (uint32 plane = 0; plane < dstImage.Planes (); plane++)
			{
		
			scaleTransforms [plane] = dng_matrix_3by3 (1.0 / downScale.v, 0.0, 0.0,
													   0.0, 1.0 / downScale.h, 0.0,
													   0.0, 0.0, 1.0);
				
			}
		
		}
	
	if (downScale == dng_point (1, 1))
		{
	
		InterpolateGeneric (host,
							negative,
							srcImage,
							dstImage,
							srcPlane);
							
		}
		
	else
		{
		
		InterpolateFast (host,
						 negative,
						 srcImage,
						 dstImage,
						 downScale,
						 srcPlane);
		
		}
	
	}

bool dng_mosaic_info::SupportsPreservedBlackLevels () const
	{
	
	return false;
	
	}

