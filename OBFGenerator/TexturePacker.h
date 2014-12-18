#pragma once



struct TextureBlock
{
    uint32_t TextureID;
    RECT Subrect;
    float XOffset;
    float YOffset;
    float XAdvance;
	bool flipped;
};



class TexturePacker
{
private:

struct rect_ltrb;
struct rect_xywh;

struct rect_wh {
	rect_wh(const rect_ltrb&);
	rect_wh(const rect_xywh&);
	rect_wh(int w = 0, int h = 0);
	int w,h, area(), perimeter(), 
		fits(const rect_wh& bigger) const; // 0 - no, 1 - yes, 2 - flipped, 3 - perfectly, 4 perfectly flipped
};

// rectangle implementing left/top/right/bottom behaviour

struct rect_ltrb {
	rect_ltrb();
	rect_ltrb(int left, int top, int right, int bottom);
	int l, t, r, b, w() const, h() const, area() const, perimeter() const;
	void w(int), h(int);
};

struct rect_xywh : public rect_wh {
	rect_xywh();
	rect_xywh(const rect_ltrb&);
	rect_xywh(int x, int y, int width, int height);
	operator rect_ltrb();

	int x, y, r() const, b() const;
	void r(int), b(int);
};

struct rect_xywhf : public rect_xywh {
	rect_xywhf(const rect_ltrb&);
	rect_xywhf(int x, int y, int width, int height);
	rect_xywhf();
	void flip();
	bool flipped;
};


struct node {
	struct pnode {
		node* pn;
		bool fill;

		pnode();
		void set(int l, int t, int r, int b);
	};

	pnode c[2];
	rect_ltrb rc;
	bool id;
	node(rect_ltrb rc = rect_ltrb());

	void reset(const rect_wh& r);

	node* insert(rect_xywhf& img); 


	void delcheck();

	~node();
};

struct bin {
	rect_wh size;
	std::vector<rect_xywhf*> rects;
};

//template <typename typeArg>
//struct compType
//{
//	bool operator() (rect_xywhf* a, rect_xywhf* b);
//};

bool area(rect_xywhf* a, rect_xywhf* b);

bool perimeter(rect_xywhf* a, rect_xywhf* b);

bool max_side(rect_xywhf* a, rect_xywhf* b);

bool max_width(rect_xywhf* a, rect_xywhf* b);

bool max_height(rect_xywhf* a, rect_xywhf* b);

bool pack(rect_xywhf* const * v, int n, int max_side, std::vector<bin>& bins);


rect_wh _rect2D(rect_xywhf* const * v, int n, int max_s, std::vector<rect_xywhf*>& succ, std::vector<rect_xywhf*>& unsucc);

typedef  bool (TexturePacker::*compareFN)(TexturePacker::rect_xywhf*, TexturePacker::rect_xywhf*);

int discard_step;
compareFN cmpf[5];

public:
	TexturePacker(void);
	~TexturePacker(void);

	bool packTexture(std::vector<std::shared_ptr<const TextureBlock>>& inputTextures, int maxSideSize, std::vector<std::shared_ptr<TextureBlock>>& outData);

};


