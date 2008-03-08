#include "faster_bytecode.h"
#include <cvd/image.h>
#include <cerrno>


using namespace CVD;
using namespace std;

#ifndef NOJIT
#include <sys/mman.h>
///This struct contains a x86 machine-code compiled version of the detector. The detector
///operates on a single row and inserts offset from the beginning of the image in to a 
///std::vector. 
///@ingroup gFastTree
class jit_detector
{
	public:
		

		///Run the compiled detector on a row of an image.
		///@param im The image.
		///@param row The row to detect corners in.
		///@param xmin The starting position.
		///@param  xmax The ending position.
		///@param corners The detected corners as offsets from image.data().
		///@param threshold The corner detector threshold.
		void detect_in_row(const Image<byte>& im, int row, int xmin, int xmax, vector<int>& corners, int threshold)
		{

			const byte* p = im[row] + xmin;
			const int n = xmax - xmin;
			void* cs = &corners;
			const void* im_data = im.data();
			/* r/m usage, at entry to machine code
				In use:
					%ecx				Num remaining
					%edi 				threshold
					%ebp 			    Detect in row machine code procedure address
					%ebx				cb
					%edx				c_b
					%esi				data
					%eax				Scratch

					4%esp 				%esi: produced automatically by call
					8%esp				image.data()
					12%esp				&vector<int>
					16%esp				vector_inserter: simple function for calling member of std::vector


				Input:	
					0 num remaining
					1 data pointer
					2 threshold
					3 proc 
					4 push_back_proc
					5 vector<int>
					6 image.data()
			*/

			__asm__ __volatile__(
				//Save all registers
				"	pusha								\n"
				
				//Load operands in to correct places
				"	pushl			%4					\n"
				"	pushl			%5					\n"
				"	pushl			%6					\n"
				"	movl			%0, %%ecx			\n"	
				"	movl			%1, %%esi			\n"
				"	movl			%2, %%edi			\n"
				" 	movl			%3, %%ebp			\n"   //%? uses ebp, so trash ebp last

				
				//Start the loop
				"	cmp				$0, %%ecx			\n"
				"	je				1					\n"
				"	call			*%%ebp				\n"
				"1:										\n"


				//Unload operands
				"	popl			%%eax				\n"
				"	popl			%%eax				\n"
				"	popl			%%eax				\n"

				//Restore all registers
				"	popa								\n"
				:
				: "m"(n), "m"(p), "m"(threshold), "m"(proc), "i"(&vector_inserter), "m"(cs), "m"(im_data)
			);


		}


		///Create a compiled detector from the bytecode.
		///@param v Bytecode.
		jit_detector(const vector<block_bytecode::fast_detector_bit>& v)
		{
			//blocksize
			const int bs=28;

			length = bs * (v.size() + 2); //Add head and tail block

			/* The original assembler code looked like this
			   This is now done in machine code, with the whole tree in
			   place of  line 0x804e0c1.

			 804e0b3:	83 f9 00             	cmp    $0x0,%ecx
			 804e0b6:	74 1b                	je     804e0d3 <finished>

			0804e0b8 <loop>:
			 804e0b8:	0f b6 16             	movzbl (%esi),%edx
			 804e0bb:	89 d3                	mov    %edx,%ebx
			 804e0bd:	29 fa                	sub    %edi,%edx
			 804e0bf:	01 fb                	add    %edi,%ebx
			 804e0c1:	ff d5                	call   *%ebp
			 804e0c3:	a8 ff                	test   $0xff,%al
			 804e0c5:	74 08                	je     804e0cf <nocorner>
			 804e0c7:	56                   	push   %esi
			 804e0c8:	51                   	push   %ecx
			 804e0c9:	ff 54 24 10          	call   *0x10(%esp)
			 804e0cd:	59                   	pop    %ecx
			 804e0ce:	58                   	pop    %eax

			0804e0cf <nocorner>:
			 804e0cf:	46                   	inc    %esi
			 804e0d0:	49                   	dec    %ecx
			 804e0d1:	75 e5                	jne    804e0b8 <loop>			//jne == jnz

		  	Unused spaces are filled in with int $3, (instruction 0xcc), which
			causes a debug trap. Makes catching errors easier.
			
			The consists of fixed sized blocks pasted together. The size is determined by the
			largest block, which is a tree node. This makes jump computation trivial, but 
			it also means that short jumps are never used, and the code is therefore larger
			than necessary.

			The rest have 0xcc filled in in the spare places. 

			The blocks are templates and have the relevant parts filled in prior to 
			copying.

			Each tree node (including leaves are represented by an entire block)

			Detectod corners are inserted in to a vector<int> as the integer offset of the corner
			pixel from the beginning of the image
			*/

			const unsigned char loop_head[bs] = 
			{
				0xEB, 0x11,							//jmp + 17

				0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,		//dead space
				0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,
				0xcc,0xcc,0xcc,0xcc,0xcc,


				0x0f, 0xb6, 0x16,					//movzbl (%esi),%edx			Load data
				0x89, 0xd3,                			//mov    %edx,%ebx
				0x29, 0xfa,                			//sub    %edi,%edx				Compute c_b
				0x01, 0xfb,                			//add    %edi,%ebx				Compute cb
			};
			const int loop_head_start=19;			//Jump to here to continue loop


			unsigned char loop_tail[bs] = 
			{
				0x56,								//push %esi				Functions seem to trash this otherwise
				0x51,								//push %ecx				Functions seem to trash this otherwise
				0xFF, 0x54, 0x24, 0x14,				//call *16(%esp)		Other arguments on the stack already
				0x59,								//pop %ecx				Clean stack
				0x58,								//pop %eax				...
				
				0x46,								//inc %esi
				0x49,								//dec %ecx
				0x0F, 0x85, 0xcc, 0xcc, 0xcc, 0xcc,	//jnz <back to first block>

				0xc3, 								//ret
				0xcc,0xcc,0xcc,0xcc,  				//dead space 
				0xcc,0xcc,0xcc,0xcc,
				0xcc,0xcc,0xcc,
			};
			const int loop_tail_address_offset = 12;   //fill in the jump <back to first block> address here
			const int loop_tail_jump_delta     = 16;   //Jump block_size*depth + this, to loop.
			const int loop_tail_entry		   = 8;    //jump to here to avoid inserting current point as corner

			unsigned char cont_or_goto[bs] = 
			{	
				0xE9,0xcc, 0xcc, 0xcc, 0xcc,		//Jump to end of loop
				0xcc,0xcc,0xcc,0xcc,0xcc,0xcc, 		//dead space
				0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,
				0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,
				0xcc,0xcc,0xcc,0xcc,0xcc
			};
			const int cont_jmp_addr = 1;			//Jump address filled in here
			const int cont_delta = 5;				//This much in addition to block delta
			
			unsigned char branch[bs] = 
			{
				0x0f, 0xB6, 0x86, 0xcc, 0xcc, 0xcc, 0xcc, 	//movzbl   OOOO(%esi),%eax
				0x39, 0xd8,									//cmp      %ebx, %eax   (eax - ebx) = (data[##]-cb
				0x0F, 0x8F, 0xcc, 0xcc, 0xcc, 0xcc,			//jg       XXXX         jmp by XXXX if eax > ebx
				0x39, 0xC2,									//cmp      %eax, %edx   (edx - eax) = c_b - data[##]
				0x0F, 0x8F, 0xcc, 0xcc, 0xcc, 0xcc,			//jg       YYYY         jmp by YYYY if ecx > ebx
				0xE9, 0xcc, 0xcc, 0xcc, 0xcc,				//jmp	   ZZZZ			Unconditional jump to ZZZZ
			};
			const int block_off_off = 3;
			const int block_gt_off = 11;
			const int block_lt_off = 19;
			const int block_eq_off = 24;


			//mmap a writable, executable block of memory for JITted code
			proc = (unsigned char*) mmap(0, length, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
			if(proc == MAP_FAILED)
			{
				cerr << "mmap failed with error: " << strerror(errno) << endl;
				exit(1);
			}
			
			//Copy in the loop head: no parts to be filled in.
			memcpy(proc, loop_head, bs);

			for(int i=0; i < (int)v.size(); i++)
			{
				if(v[i].lt == 0)			// leaf
				{
					if(v[i].gt == 0) //Fill in jump to continue part
					{
						*(int*)(cont_or_goto + cont_jmp_addr) = bs * (v.size()- i) - cont_delta + loop_tail_entry;
					}
					else //fill in jump to insert part
					{
						*(int*)(cont_or_goto + cont_jmp_addr) = bs * (v.size() - i) - cont_delta;
					}
						

					memcpy(proc + (i+1)*bs, cont_or_goto, bs);
				}
				else
				{
					*(int*)(branch+block_off_off)  = v[i].offset;

					//Optimization: leaf nodes have a non-conditional goto in them
					//so goto the right place directly, rather than the leaf node.
					//This has a 5% effect or so, so bigger gains elsewhere.
					//Removed for simplicity.
					
					*(int*)(branch+block_gt_off) = (v[i].gt -i) * bs - (block_gt_off + 4);
					*(int*)(branch+block_lt_off) = (v[i].lt -i) * bs - (block_lt_off + 4);
					*(int*)(branch+block_eq_off) = (v[i].eq -i) * bs - (block_eq_off + 4);

					memcpy(proc + (i+1)*bs, branch, bs);
				}
			}
			
			//Insert the correct backwards jump for looping
			*(int*)(loop_tail+loop_tail_address_offset) = -bs * (1+v.size()) - loop_tail_jump_delta + loop_head_start;
			memcpy(proc + bs * (v.size() + 1), loop_tail, bs);

		}


		~jit_detector()
		{
			munmap(proc, length);
		}

	private:
		//Not copyable
		void operator=(const jit_detector&);
		jit_detector(const jit_detector&);

		unsigned char* proc;			///< The machine code is stored in this mmap() allocated data which allows code execution.
		int			   length;			///< Number of mmap() allocated bytes.

		///Callback function to allow insertion in to std::vector. The execution of this function
		///relies on the stack having the following layout (stack head on the left):
		///@code
		///return_address first_arg second_arg etc...
		///@endcode
		///so that the arguemnts directly reflect the stack layout.
		///For speed, and in order to minimize stack handling, the argument list spans two call instructions worth of stack.
		///
		///@param ecx_dummy Pushed by the machine code, since the ABI allows ecx to be trashed
		///@param p The pointer to the current pixel. Pushed by the machine code.
		///@param esp_return_dummy Location to return to on a return from the machine code. Generated by the assembler call in to the machine code.
		///@param im_data Pointer to the first image pixel. Pushed by the assembler caller.
		///@param i  Pointer to the std::vector<int> which stores the data. Pushed by the assembler caller.
		static void vector_inserter(int ecx_dummy, const byte* p, const void* esp_return_dummy, const byte* im_data, vector<int>* i)
		{
			i->push_back(p-im_data);
		}
};
#endif

void block_bytecode::detect(const CVD::Image<CVD::byte>& im, std::vector<int>& corners, int threshold, int xmin, int xmax, int ymin, int ymax)
{
	#ifdef NOJIT
	for(int y = ymin; y < ymax; y++)
		for(int x=xmin; x < xmax; x++)
			if(detect_no_score(&im[y][x], threshold))
				corners.push_back(&im[y][x] - im.data());
	#else
		jit_detector jit(d);
		for(int y = ymin; y < ymax; y++)
			jit.detect_in_row(im, y, xmin, xmax, corners, threshold);
	#endif
}

