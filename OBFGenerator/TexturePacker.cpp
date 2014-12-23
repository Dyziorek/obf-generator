#include "stdafx.h"
#include "TexturePacker.h"

using namespace std;

#pragma push_macro(max)
#undef max

// just add another comparing function name to cmpf to perform another packing attempt
// more functions == slower but probably more efficient cases covered and hence less area wasted



// if you find the algorithm running too slow you may double this factor to increase speed but also decrease efficiency
// 1 == most efficient, slowest
// efficiency may be still satisfying at 64 or even 256 with nice speedup



/*

For every sorting function, algorithm will perform packing attempts beginning with a bin with width and height equal to max_side,
and decreasing its dimensions if it finds out that rectangles did actually fit, increasing otherwise.
Although, it's doing that in sort of binary search manner, so for every comparing function it will perform at most log2(max_side) packing attempts looking for the smallest possible bin size.
discard_step means that if the algorithm will break of the searching loop if the rectangles fit but "it may be possible to fit them in a bin smaller by discard_step"  

may be pretty slow in debug mode anyway (std::vector and stuff like that in debug mode is always slow)

the algorithm was based on http://www.blackpawn.com/texts/lightmaps/default.html
the algorithm reuses the node tree so it doesn't reallocate them between searching attempts


please let me know about bugs at  <----
unknownunreleased@gmail.com           |
|
|
ps. I'm 16 so take this --------------- more than seriously, though I made some realtime tests with packing several hundreds of rectangles every frame, no crashes, no memory leaks, good results
Thank you.
*/

/*************************************************************************** CHAOS BEGINS HERE */


TexturePacker::TexturePacker(void)
{
	//compareFN cmpf[5];
	cmpf[0] = &TexturePacker::area;
	cmpf[1] = &TexturePacker::perimeter;
	cmpf[2] = &TexturePacker::max_side;
	cmpf[3] = &TexturePacker::max_width;
	cmpf[4] = &TexturePacker::max_height;

	discard_step = 128;
}


TexturePacker::~TexturePacker(void)
{
}

bool TexturePacker::area(TexturePacker::rect_xywhf* a, TexturePacker::rect_xywhf* b) {
	return a->area() > b->area();
}

bool TexturePacker::perimeter(TexturePacker::rect_xywhf* a, TexturePacker::rect_xywhf* b) {
	return a->perimeter() > b->perimeter();
}

bool TexturePacker::max_side(TexturePacker::rect_xywhf* a, TexturePacker::rect_xywhf* b) {
	return max(a->w, a->h) > max(b->w, b->h);
}

bool TexturePacker::max_width(rect_xywhf* a, rect_xywhf* b) {
	return a->w > b->w;
}

bool TexturePacker::max_height(rect_xywhf* a, rect_xywhf* b) {
	return a->h > b->h;
}



TexturePacker::node::pnode::pnode() : fill(false), pn(0)
{
}

void TexturePacker::node::pnode::set(int l, int t, int r, int b) {
	if(!pn) pn = new node(rect_ltrb(l, t, r, b));
	else {
		(*pn).rc = rect_ltrb(l, t, r, b);
		(*pn).id = false;
	}
	fill = true;
}

TexturePacker::node::node(rect_ltrb rc) : id(false), rc(rc) 
{
}

void TexturePacker::node::reset(const rect_wh& r) {
		id = false;
		rc = rect_ltrb(0, 0, r.w, r.h);
		delcheck();
	}

TexturePacker::node* TexturePacker::node::insert(rect_xywhf& img) 
{
		if(c[0].pn && c[0].fill) {
			node* newn;
			if(newn = c[0].pn->insert(img)) return newn;
			return    c[1].pn->insert(img);
		}

		if(id) return 0;
		int f = img.fits(rect_xywh(rc));

		switch(f) {
		case 0: return 0;
		case 1: img.flipped = false; break;
		case 2: img.flipped = true; break;
		case 3: id = &img; img.flipped = false; return this;
		case 4: id = &img; img.flipped = true;  return this;
		}

		int iw = (img.flipped ? img.h : img.w), ih = (img.flipped ? img.w : img.h);

		if(rc.w() - iw > rc.h() - ih) {
			c[0].set(rc.l, rc.t, rc.l+iw, rc.b);
			c[1].set(rc.l+iw, rc.t, rc.r, rc.b);
		}
		else {
			c[0].set(rc.l, rc.t, rc.r, rc.t + ih);
			c[1].set(rc.l, rc.t + ih, rc.r, rc.b);
		}

		return c[0].pn->insert(img);
}

void TexturePacker::node::delcheck() 
{
	if(c[0].pn) { c[0].fill = false; c[0].pn->delcheck(); }
	if(c[1].pn) { c[1].fill = false; c[1].pn->delcheck(); }
}

TexturePacker::node::~node() 
{
	if(c[0].pn) delete c[0].pn;
	if(c[1].pn) delete c[1].pn;
}


TexturePacker::rect_wh TexturePacker::_rect2D(rect_xywhf* const * v, int n, int max_s, vector<rect_xywhf*>& succ, vector<rect_xywhf*>& unsucc) {
	node root;

	const int funcs = sizeof(cmpf)/sizeof(compareFN);

	rect_xywhf** order[funcs];

	for(int f = 0; f < funcs; ++f) { 
		order[f] = new rect_xywhf*[n];
		memcpy(order[f], v, sizeof(rect_xywhf*) * n);
		auto meberFN = cmpf[f];
		sort(order[f], order[f]+n, bind(meberFN, this, placeholders::_1, placeholders::_2));
	}

	rect_wh min_bin = rect_wh(max_s, max_s);
	int min_func = -1, best_func = 0, best_area = 0, _area = 0, step, fit, i;

	bool fail = false;

	for(int f = 0; f < funcs; ++f) {
		v = order[f];
		step = min_bin.w / 2;
		root.reset(min_bin);

		while(true) {
			if(root.rc.w() > min_bin.w) {
				if(min_func > -1) break;
				_area = 0;

				root.reset(min_bin);
				for(i = 0; i < n; ++i)
					if(root.insert(*v[i]))
						_area += v[i]->area();

				fail = true;
				break;
			}

			fit = -1;

			for(i = 0; i < n; ++i)
				if(!root.insert(*v[i])) {
					fit = 1;
					break;
				}

				if(fit == -1 && step <= discard_step)
					break;

				root.reset(rect_wh(root.rc.w() + fit*step, root.rc.h() + fit*step));

				step /= 2;
				if(!step) 
					step = 1;
		}

		if(!fail && (min_bin.area() >= root.rc.area())) {
			min_bin = rect_wh(root.rc);
			min_func = f;
		}

		else if(fail && (_area > best_area)) {
			best_area = _area;
			best_func = f;
		}
		fail = false;
	}

	v = order[min_func == -1 ? best_func : min_func];

	int clip_x = 0, clip_y = 0;
	node* ret;

	root.reset(min_bin);

	for(i = 0; i < n; ++i) {
		if(ret = root.insert(*v[i])) {
			v[i]->x = ret->rc.l;
			v[i]->y = ret->rc.t;

			if(v[i]->flipped) {
				v[i]->flipped = false;
				v[i]->flip();
			}

			clip_x = max(clip_x, ret->rc.r);
			clip_y = max(clip_y, ret->rc.b); 

			succ.push_back(v[i]);
		}
		else {
			unsucc.push_back(v[i]);

			v[i]->flipped = false;
		}
	}

	for(int f = 0; f < funcs; ++f)
		delete [] order[f];

	return rect_wh(clip_x, clip_y);
}


bool TexturePacker::pack(rect_xywhf* const * v, int n, int max_s, vector<bin>& bins) {
	rect_wh _rect(max_s, max_s);

	for(int i = 0; i < n; ++i) 
		if(!v[i]->fits(_rect)) return false;

	vector<rect_xywhf*> vec[2], *p[2] = { vec, vec+1 };
	vec[0].resize(n);
	vec[1].clear();
	memcpy(&vec[0][0], v, sizeof(rect_xywhf*)*n);

	bin* b = 0;

	while(true) {
		bins.push_back(bin());
		b = &bins[bins.size()-1];

		b->size = _rect2D(&((*p[0])[0]), p[0]->size(), max_s, b->rects, *p[1]);
		b->rects.shrink_to_fit();
		p[0]->clear();

		if(!p[1]->size()) break;

		std::swap(p[0], p[1]);
	}

	return true;
}


TexturePacker::rect_wh::rect_wh(const rect_ltrb& rr) : w(rr.w()), h(rr.h()) {} 
TexturePacker::rect_wh::rect_wh(const rect_xywh& rr) : w(rr.w), h(rr.h) {} 
TexturePacker::rect_wh::rect_wh(int w, int h) : w(w), h(h) {}

int TexturePacker::rect_wh::fits(const rect_wh& r) const {
	if(w == r.w && h == r.h) return 3;
	if(h == r.w && w == r.h) return 4;
	if(w <= r.w && h <= r.h) return 1;
	if(h <= r.w && w <= r.h) return 2;
	return 0;
}

TexturePacker::rect_ltrb::rect_ltrb() : l(0), t(0), r(0), b(0) {}
TexturePacker::rect_ltrb::rect_ltrb(int l, int t, int r, int b) : l(l), t(t), r(r), b(b) {}

int TexturePacker::rect_ltrb::w() const {
	return r-l;
}

int TexturePacker::rect_ltrb::h() const {
	return b-t;
}

int TexturePacker::rect_ltrb::area() const {
	return w()*h();
}

int TexturePacker::rect_ltrb::perimeter() const {
	return 2*w() + 2*h();
}

void TexturePacker::rect_ltrb::w(int ww) {
	r = l+ww;
}

void TexturePacker::rect_ltrb::h(int hh) {
	b = t+hh;
}

TexturePacker::rect_xywh::rect_xywh() : x(0), y(0) {}
TexturePacker::rect_xywh::rect_xywh(const rect_ltrb& rc) : x(rc.l), y(rc.t) { b(rc.b); r(rc.r); }
TexturePacker::rect_xywh::rect_xywh(int x, int y, int w, int h) : x(x), y(y), rect_wh(w, h) {}

TexturePacker::rect_xywh::operator TexturePacker::rect_ltrb() {
	rect_ltrb rr(x, y, 0, 0);
	rr.w(w); rr.h(h);
	return rr;
}

int TexturePacker::rect_xywh::r() const {
	return x+w;
};

int TexturePacker::rect_xywh::b() const {
	return y+h;
}

void TexturePacker::rect_xywh::r(int right) {
	w = right-x;
}

void TexturePacker::rect_xywh::b(int bottom) {
	h = bottom-y;
}

int TexturePacker::rect_wh::area() {
	return w*h;
}

int TexturePacker::rect_wh::perimeter() {
	return 2*w + 2*h; 
}


TexturePacker::rect_xywhf::rect_xywhf(const rect_ltrb& rr) : rect_xywh(rr), flipped(false), id(-1) {}
TexturePacker::rect_xywhf::rect_xywhf(int x, int y, int width, int height, int handle) : rect_xywh(x, y, width, height), flipped(false), id(handle) {}
TexturePacker::rect_xywhf::rect_xywhf() : flipped(false) {}

void TexturePacker::rect_xywhf::flip() { 
	flipped = !flipped;
	std::swap(w, h);
}

bool TexturePacker::packTexture(std::vector<const std::shared_ptr<const TextureBlock>>& inputTextures, int maxSideSize, std::vector<std::shared_ptr<TextureBlock>>& outData)
{
	std::vector<rect_xywhf*> rectPack;

	for_each(inputTextures.begin(), inputTextures.end(), [&rectPack](std::shared_ptr<const TextureBlock> arg) {
		rectPack.push_back(new rect_xywhf(arg->Subrect.left, arg->Subrect.top, arg->Subrect.right, arg->Subrect.bottom, arg->textureHandle));
	});
	
	vector<bin> bins;

	bool bOK = pack(rectPack.data(), rectPack.size(), maxSideSize, bins);
	if (bOK)
	{
		for (int binID = 0; binID < bins.size(); binID++)
		{
			for (auto rectData : bins[binID].rects)
			{
				TextureBlock* newBlock = new TextureBlock();
				newBlock->textureID = binID;
				newBlock->textureHandle = rectData->id;
				newBlock->XOffset = rectData->x;
				newBlock->YOffset = rectData->y;
				newBlock->Subrect.left = 0;
				newBlock->Subrect.top = 0;
				newBlock->Subrect.right = rectData->w;
				newBlock->Subrect.bottom = rectData->h;
				newBlock->flipped = rectData->flipped;
				outData.push_back(std::shared_ptr<TextureBlock>(newBlock));
			}
		}
	}
	return bOK;
}

#pragma pop_macro(max)