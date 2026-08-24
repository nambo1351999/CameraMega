

#include "dng_area_task.h"

#include "dng_abort_sniffer.h"
#include "dng_assertions.h"
#include "dng_auto_ptr.h"
#include "dng_flags.h"
#include "dng_globals.h"
#include "dng_host.h"
#include "dng_image.h"
#include "dng_pixel_buffer.h"
#include "dng_safe_arithmetic.h"
#include "dng_sdk_limits.h"
#include "dng_tile_iterator.h"
#include "dng_utils.h"

dng_area_task::dng_area_task (const char *name)

	:	fMaxThreads	  (kMaxMPThreads)
	
	,	fMinTaskArea  (256 * 256)
	
	,	fUnitCell	  (1, 1)
	
	,	fMaxTileSize  (256, 256)

	,	fName ()

	{

	if (!name)
		{
		name = "dng_area_task";
		}

	fName.Set (name);
	
	}

dng_area_task::~dng_area_task ()
	{
	
	}

dng_rect dng_area_task::RepeatingTile1 () const
	{
	
	return dng_rect ();
	
	}
		

dng_rect dng_area_task::RepeatingTile2 () const
	{
	
	return dng_rect ();
	
	}
		

dng_rect dng_area_task::RepeatingTile3 () const
	{
	
	return dng_rect ();
	
	}
		

void dng_area_task::Start (uint32 ,
						   const dng_rect & ,
						   const dng_point & ,
						   dng_memory_allocator * ,
						   dng_abort_sniffer * )
	{
	
	}

void dng_area_task::Finish (uint32 )
	{
	
	}
	

dng_point dng_area_task::FindTileSize (const dng_rect &area) const
	{
	
	dng_rect repeatingTile1 = RepeatingTile1 ();
	dng_rect repeatingTile2 = RepeatingTile2 ();
	dng_rect repeatingTile3 = RepeatingTile3 ();
	
	if (repeatingTile1.IsEmpty ())
		{
		repeatingTile1 = area;
		}
	
	if (repeatingTile2.IsEmpty ())
		{
		repeatingTile2 = area;
		}
	
	if (repeatingTile3.IsEmpty ())
		{
		repeatingTile3 = area;
		}
		
	uint32 repeatV = Min_uint32 (Min_uint32 (repeatingTile1.H (),
											 repeatingTile2.H ()),
											 repeatingTile3.H ());
	
	uint32 repeatH = Min_uint32 (Min_uint32 (repeatingTile1.W (),
											 repeatingTile2.W ()),
											 repeatingTile3.W ());
	
	dng_point maxTileSize = MaxTileSize ();

	dng_point tileSize;
		
	tileSize.v = Min_int32 (repeatV, maxTileSize.v);
	tileSize.h = Min_int32 (repeatH, maxTileSize.h);
	
	

	tileSize.v = Max_int32 (tileSize.v, 1);
	tileSize.h = Max_int32 (tileSize.h, 1);

	
	
	
	
	
						
	
	
	

	uint32 countV = (repeatV + tileSize.v - 1) / tileSize.v;
	uint32 countH = (repeatH + tileSize.h - 1) / tileSize.h;

	

	countV = Max_uint32 (countV, 1);
	countH = Max_uint32 (countH, 1);
	
	tileSize.v = (repeatV + countV - 1) / countV;
	tileSize.h = (repeatH + countH - 1) / countH;
	
	
	
	dng_point unitCell = UnitCell ();
	
	if (unitCell.h != 1 || unitCell.v != 1)
		{
		tileSize.v = ((tileSize.v + unitCell.v - 1) / unitCell.v) * unitCell.v;
		tileSize.h = ((tileSize.h + unitCell.h - 1) / unitCell.h) * unitCell.h;
		}
		
	
	
	if (tileSize.v > maxTileSize.v)
		{
		tileSize.v = (maxTileSize.v / unitCell.v) * unitCell.v;
		}

	if (tileSize.h > maxTileSize.h)
		{
		tileSize.h = (maxTileSize.h / unitCell.h) * unitCell.h;
		}
		
	if (gPrintTimings)
		{
		fprintf (stdout,
				 "\nRender tile for below: %d x %d\n",
				 (int32) tileSize.h,
				 (int32) tileSize.v);
		}	

	return tileSize;
	
	}
		

void dng_area_task::ProcessOnThread (uint32 threadIndex,
									 const dng_rect &area,
									 const dng_point &tileSize,
									 dng_abort_sniffer *sniffer,
									 dng_area_task_progress *progress)
	{

	dng_rect repeatingTile1 = RepeatingTile1 ();
	dng_rect repeatingTile2 = RepeatingTile2 ();
	dng_rect repeatingTile3 = RepeatingTile3 ();
	
	if (repeatingTile1.IsEmpty ())
		{
		repeatingTile1 = area;
		}
	
	if (repeatingTile2.IsEmpty ())
		{
		repeatingTile2 = area;
		}
	
	if (repeatingTile3.IsEmpty ())
		{
		repeatingTile3 = area;
		}
		
	dng_rect tile1;

	
	AutoPtr<dng_base_tile_iterator> iter1
		(MakeTileIterator (threadIndex,
						   repeatingTile3, 
						   area));
	
	while (iter1->GetOneTile (tile1))
		{
		
		dng_rect tile2;
		
		AutoPtr<dng_base_tile_iterator> iter2
			(MakeTileIterator (threadIndex,
							   repeatingTile2, 
							   tile1));
	
		while (iter2->GetOneTile (tile2))
			{
			
			dng_rect tile3;
			
			AutoPtr<dng_base_tile_iterator> iter3
				(MakeTileIterator (threadIndex,
								   repeatingTile1, 
								   tile2));
			
			while (iter3->GetOneTile (tile3))
				{
				
				dng_rect tile4;
				
				AutoPtr<dng_base_tile_iterator> iter4
					(MakeTileIterator (threadIndex,
									   tileSize, 
									   tile3));
				
				while (iter4->GetOneTile (tile4))
					{
					
					dng_abort_sniffer::SniffForAbort (sniffer);
					
					Process (threadIndex, tile4, sniffer);

					if (progress)
						{
						progress->FinishedTile (tile4);
						}
					
					}
					
				}
				
			}
		
		}

	}

dng_base_tile_iterator * dng_area_task::MakeTileIterator (uint32 ,
														  const dng_rect &tile,
														  const dng_rect &area) const
	{
	
	return new dng_tile_forward_iterator (tile, area);
	
	}
		

dng_base_tile_iterator * dng_area_task::MakeTileIterator (uint32 ,
														  const dng_point &tileSize,
														  const dng_rect &area) const
	{
	
	return new dng_tile_forward_iterator (tileSize, area);
	
	}
		

void dng_area_task::Perform (dng_area_task &task,
							 const dng_rect &area,
							 dng_memory_allocator *allocator,
							 dng_abort_sniffer *sniffer,
							 dng_area_task_progress *progress)
	{
	
	dng_point tileSize (task.FindTileSize (area));
		
	task.Start (1, area, tileSize, allocator, sniffer);
	
	try
		{
		task.ProcessOnThread (0, area, tileSize, sniffer, progress);
		}

	catch (...)
		{
		task.Finish (1);
		throw;
		}

	task.Finish (1);
	
	}

dng_range_parallel_task::dng_range_parallel_task (dng_host &host,
												  int32 startIndex,
												  int32 stopIndex,
												  const char *name)

	:	dng_area_task ((name != NULL) ? name : "dng_range_parallel_task")

	,	fHost		(host)
	,	fStartIndex (startIndex)
	,	fStopIndex	(stopIndex)
	,	fIndices	()

	{

	DNG_REQUIRE (stopIndex > startIndex,
				 "Invalid start/stop index values");

	fMinTaskArea = kDummySize * kDummySize;
	fUnitCell	 = dng_point (kDummySize, kDummySize);
	fMaxTileSize = dng_point (kDummySize, kDummySize);

	}

uint32 dng_range_parallel_task::RecommendedThreadCount () const
	{

	return fHost.PerformAreaTaskThreads ();

	}

int32 dng_range_parallel_task::MinIndicesPerThread () const
	{

	return 1;

	}

void dng_range_parallel_task::Run ()
	{

	uint32 threadCount = fHost.PerformAreaTaskThreads ();

	threadCount = Min_uint32 (threadCount, 
							  RecommendedThreadCount ());

	
	
	
	
	
	

	const int32 items = SafeInt32Sub (fStopIndex, fStartIndex);

	

	int32 minItemsPerTask = Max_int32 (1, MinIndicesPerThread ());

	
	
	int32 tasks = Max_int32 (items / minItemsPerTask, 1);

	

	threadCount = Min_uint32 (threadCount,
							  (uint32) tasks);

	
	
	

	real64 itemsPerThread64 = items / (real64) threadCount;

	

	fIndices.resize ((size_t) (threadCount + 1));

	

	real64 idx64 = 0.0;

	for (uint32 i = 0; i <= threadCount; i++)
		{

		int32 idx = fStartIndex + Round_int32 (idx64);
		
		fIndices [i] = idx;

		idx64 += itemsPerThread64;
		
		}

	fHost.PerformAreaTask (*this, 
						   dng_rect (0, 
									 0, 
									 kDummySize, 
									 kDummySize * threadCount));

	}

void dng_range_parallel_task::Prepare (uint32 ,
									   dng_memory_allocator * ,
									   dng_abort_sniffer * )
	{

	}

void dng_range_parallel_task::Start (uint32 threadCount,
									 const dng_rect & ,
									 const dng_point & ,
									 dng_memory_allocator *allocator,
									 dng_abort_sniffer *sniffer)
	{

	Prepare (threadCount, 
			 allocator, 
			 sniffer);

	}
	

void dng_range_parallel_task::Process (uint32 ,
									   const dng_rect &tile,
									   dng_abort_sniffer *sniffer)
	{

	
	

	int32 index0 = tile.l / kDummySize;
	int32 index1 = index0 + 1;

	

	if (index0 < 0 || index1 >= (int32) fIndices.size ())
		{
		return;
		}

	int32 startIndex = fIndices [index0];
	int32 stopIndex	 = fIndices [index1];

	uint32 threadIndex = (uint32) index0;

	

	ProcessRange (threadIndex,
				  startIndex,
				  stopIndex,
				  sniffer);

	}

class dng_range_parallel_func_task: public dng_range_parallel_task
	{

	private:

		const uint32 fMinIndicesPerThread;

		const uint32 fRecommendedThreadCount;

		const dng_range_parallel_task::function_t &fFunc;

	public:
		
		dng_range_parallel_func_task (dng_host &host,
									  const info &params,								 
									  const char *taskName,
									  const dng_range_parallel_task::function_t &func)

			:	dng_range_parallel_task (host,
										 params.fBegin,
										 params.fEnd,
										 taskName)

			,	fMinIndicesPerThread (Max_uint32 (params.fMinIndicesPerThread, 1))

			,	fRecommendedThreadCount (params.fRecommendedThreadCount)

			,	fFunc (func)

			{

			}
		
		virtual uint32 RecommendedThreadCount () const
			{

			if (fRecommendedThreadCount > 0)
				{
				return fRecommendedThreadCount;
				}

			return dng_range_parallel_task::RecommendedThreadCount ();

			}

		virtual int32 MinIndicesPerThread () const
			{

			return fMinIndicesPerThread;

			}

		virtual void ProcessRange (uint32 threadIndex,
								   int32 startIndex,
								   int32 stopIndex,
								   dng_abort_sniffer *sniffer)
			{

			dng_range_parallel_task::range r;

			r.fThreadIndex = threadIndex;
			r.fBegin	   = startIndex;
			r.fEnd		   = stopIndex;
			r.fSniffer	   = sniffer;
			r.fAllocator   = &fHost.Allocator ();

			fFunc (r);

			}
		
	};

void dng_range_parallel_task::Do (dng_host &host,
								  const info &params,								 
								  const char *taskName,
								  const function_t &func)
	{

	dng_range_parallel_func_task task (host,
									   params,
									   taskName,
									   func);

	task.Run ();
	
	}

dng_rect dng_get_buffer_task::RepeatingTile1 () const
	{
			
	return fSrc.RepeatingTile ();
			
	}
			

void dng_get_buffer_task::Process (uint32 ,
								   const dng_rect &tile,
								   dng_abort_sniffer * )
	{

	dng_pixel_buffer temp = fDst;

	temp.fData = (void *) fDst.DirtyPixel (tile.t, tile.l, temp.fPlane);

	temp.fArea = tile;

	fSrc.Get (temp,
			  fEdgeOption);
		
	}

dng_copy_buffer_task::dng_copy_buffer_task (const dng_pixel_buffer &src,
											dng_pixel_buffer &dst)
			
	:	dng_area_task ("dng_copy_buffer_task")
				
	,	fSrc (src)
	,	fDst (dst)
				
	{

	DNG_REQUIRE (src.Planes () == dst.Planes (),
				 "Mismatched planes");
			
	}

dng_rect dng_copy_buffer_task::RepeatingTile1 () const
	{

	return dng_rect (128, 128);

	}
			

void dng_copy_buffer_task::Process (uint32 ,
									const dng_rect &tile,
									dng_abort_sniffer * )
	{

	fDst.CopyArea (fSrc,
				   tile,
				   0,
				   fDst.Planes ());
		
	}

dng_rect dng_put_buffer_task::RepeatingTile1 () const
	{
			
	return fDst.RepeatingTile ();
			
	}
			

void dng_put_buffer_task::Process (uint32 ,
								   const dng_rect &tile,
								   dng_abort_sniffer * )
	{

	dng_pixel_buffer temp = fSrc;

	temp.fData = (void *) fSrc.ConstPixel (tile.t,
										   tile.l,
										   temp.fPlane);

	temp.fArea = tile;

	fDst.Put (temp);
		
	}

