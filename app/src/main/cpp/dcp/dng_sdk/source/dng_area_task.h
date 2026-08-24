

#ifndef __dng_area_task__
#define __dng_area_task__

#include "dng_classes.h"
#include "dng_image.h"
#include "dng_point.h"
#include "dng_string.h"
#include "dng_types.h"
#include "dng_uncopyable.h"

#include <functional>
#include <vector>

class dng_area_task_progress: private dng_uncopyable
	{

	public:

		virtual ~dng_area_task_progress ()
			{
			}

		virtual void FinishedTile (const dng_rect & ) = 0;

	};

class dng_area_task
	{
	
	protected:

		uint32 fMaxThreads;
	
		uint32 fMinTaskArea;
		
		dng_point fUnitCell;
		
		dng_point fMaxTileSize;

		dng_string fName;
	
	public:
	
		explicit dng_area_task (const char *name = "unnamed dng_area_task");
		
		virtual ~dng_area_task ();

		const char * Name () const
			{
			return fName.Get ();
			}

		
		
		
		

		virtual uint32 MaxThreads () const
			{
			return fMaxThreads;
			}

		
		
		
		
		
		
		
		
		

		virtual uint32 MinTaskArea () const
			{
			return fMinTaskArea;
			}

		
		
		
		
		
		

		virtual dng_point UnitCell () const
			{
			return fUnitCell;
			}

		
		
		
		
		
		
		

		virtual dng_point MaxTileSize () const
			{
			return fMaxTileSize;
			}

		
		
		
		
		
		
		
		
		
		
		

		virtual dng_rect RepeatingTile1 () const;
		
		
		
		
		
		
		
		
		
		
		
		

		virtual dng_rect RepeatingTile2 () const;

		
		
		
		
		
		
		
		
		
		
		
		
		virtual dng_rect RepeatingTile3 () const;

		
		
		
		
		
		
		
		
		
		
		

		virtual void Start (uint32 threadCount,
							const dng_rect &dstArea,
							const dng_point &tileSize,
							dng_memory_allocator *allocator,
							dng_abort_sniffer *sniffer);

		
		
		
		
		
		
		
		
		
		
		
		
		

		virtual void Process (uint32 threadIndex,
							  const dng_rect &tile,
							  dng_abort_sniffer *sniffer) = 0;
		
		
		
		
		
		

		virtual void Finish (uint32 threadCount);

		
		
		

		dng_point FindTileSize (const dng_rect &area) const;

		
		
		
		
		
		
		
		
		
		

		void ProcessOnThread (uint32 threadIndex,
							  const dng_rect &area,
							  const dng_point &tileSize,
							  dng_abort_sniffer *sniffer,
							  dng_area_task_progress *progress);

		
		
		
		
		
		
		
		
		
		

		virtual dng_base_tile_iterator * MakeTileIterator (uint32 threadIndex,
														   const dng_rect &tile,
														   const dng_rect &area) const;

		
		
		
		
		
		
		
		
		
		

		virtual dng_base_tile_iterator * MakeTileIterator (uint32 threadIndex,
														   const dng_point &tileSize,
														   const dng_rect &area) const;

		
		
		
		
		
		
		
		
		
		

		static void Perform (dng_area_task &task,
							 const dng_rect &area,
							 dng_memory_allocator *allocator,
							 dng_abort_sniffer *sniffer,
							 dng_area_task_progress *progress);

	};

class dng_range_parallel_task: public dng_area_task, dng_uncopyable
	{

	private:

		static const int32 kDummySize = 16;

	protected:
	
		dng_host &fHost;

		const int32 fStartIndex;
		const int32 fStopIndex;
		
		std::vector<int32> fIndices;

	public:
	
		dng_range_parallel_task (dng_host &host,
								int32 startIndex,
								int32 stopIndex,
								const char *name = NULL);

		void Run ();

		virtual uint32 RecommendedThreadCount () const;

		virtual int32 MinIndicesPerThread () const;

		virtual void Prepare (uint32 threadCount,
							  dng_memory_allocator *allocator,
							  dng_abort_sniffer *sniffer);

		virtual void ProcessRange (uint32 threadIndex,
								   int32 startIndex,
								   int32 stopIndex,
								   dng_abort_sniffer *sniffer) = 0;

		void Start (uint32 threadCount,
					const dng_rect &dstArea,
					const dng_point &tileSize,
					dng_memory_allocator *allocator,
					dng_abort_sniffer *sniffer) override;
	
		void Process (uint32 threadIndex,
					  const dng_rect & ,
					  dng_abort_sniffer *sniffer) override;

	public:

		class info
			{

			public:

				int32  fBegin;
				int32  fEnd;
				uint32 fMinIndicesPerThread;
				uint32 fRecommendedThreadCount; 

			public:

				info (int32 begin,
					  int32 end,
					  uint32 minIndicesPerThread = 1,
					  uint32 recommendedThreadCount = 0)

					:	fBegin					(begin)
					,	fEnd					(end)
					,	fMinIndicesPerThread	(minIndicesPerThread)
					,	fRecommendedThreadCount (recommendedThreadCount)
						
					{

					}
					  
			};
		
		struct range
			{
			uint32 fThreadIndex;
			int32 fBegin;
			int32 fEnd;
			dng_abort_sniffer *fSniffer;
			dng_memory_allocator *fAllocator;
			};
		
		typedef std::function<void(const range &)> function_t;

		static void Do (dng_host &host,
						const info &params,
						const char *taskName,
						const function_t &func);
		
	};

class dng_get_buffer_task : public dng_area_task
	{
		
	private:

		const dng_image &fSrc;

		dng_pixel_buffer &fDst;

		const dng_image::edge_option fEdgeOption;

	public:

		dng_get_buffer_task (const dng_image &image,
							 dng_pixel_buffer &buffer,
							 dng_image::edge_option edgeOption = dng_image::edge_repeat)
			
			:	dng_area_task ("dng_get_buffer_task")
				
			,	fSrc (image)
			,	fDst (buffer)

			,	fEdgeOption (edgeOption)
				
			{
			
			}

		dng_rect RepeatingTile1 () const override;
			
		void Process (uint32 ,
					  const dng_rect &tile,
					  dng_abort_sniffer * ) override;
		
	};

class dng_copy_buffer_task : public dng_area_task
	{
		
	private:

		const dng_pixel_buffer &fSrc;

		dng_pixel_buffer &fDst;

	public:

		dng_copy_buffer_task (const dng_pixel_buffer &src,
							  dng_pixel_buffer &dst);

		dng_rect RepeatingTile1 () const override;
			
		void Process (uint32 ,
					  const dng_rect &tile,
					  dng_abort_sniffer * ) override;
		
	};

class dng_put_buffer_task : public dng_area_task
	{
		
	private:

		const dng_pixel_buffer &fSrc;

		dng_image &fDst;

	public:

		dng_put_buffer_task (const dng_pixel_buffer &buffer,
							 dng_image &image)
			
			:	dng_area_task ("dng_put_buffer_task")
				
			,	fSrc (buffer)
				
			,	fDst (image)
				
			{
			
			}

		dng_rect RepeatingTile1 () const override;
			
		void Process (uint32 ,
					  const dng_rect &tile,
					  dng_abort_sniffer * ) override;
		
	};

#endif	
	

