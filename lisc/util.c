#include "lisc.h"

typedef struct Vec Vec;

struct Vec {
	ulong mag;
	size_t esz;
	ulong cap;
	union {
		long long ll;
		long double ld;
		void *ptr;
	} align[];
};

enum {
	VMin = 2,
	VMag = 0xcabba9e,
};

Typ typ[NTyp];
Ins insb[NIns], *curi;

void
diag(char *s)
{
	fputs(s, stderr);
	fputc('\n', stderr);
	abort();
}

void *
alloc(size_t n)
{
	void *p;

	if (n == 0)
		return 0;
	p = calloc(1, n);
	if (!p)
		abort();
	return p;
}

Blk *
balloc()
{
	static Blk z;
	Blk *b;

	b = alloc(sizeof *b);
	*b = z;
	return b;
}

void
emit(int op, int w, Ref to, Ref arg0, Ref arg1)
{
	if (curi == insb)
		diag("emit: too many instructions");
	*--curi = (Ins){op, w, to, {arg0, arg1}};
}

void
emiti(Ins i)
{
	emit(i.op, i.wide, i.to, i.arg[0], i.arg[1]);
}

int
bcnt(Bits *b)
{
	int z, i, j;
	ulong tmp;

	i = 0;
	for (z=0; z<BITS; z++) {
		tmp = b->t[z];
		for (j=0; j<64; j++) {
			i += 1 & tmp;
			tmp >>= 1;
		}
	}
	return i;
}

void
idup(Ins **pd, Ins *s, ulong n)
{
	free(*pd);
	*pd = alloc(n * sizeof(Ins));
	memcpy(*pd, s, n * sizeof(Ins));
}

Ins *
icpy(Ins *d, Ins *s, ulong n)
{
	memcpy(d, s, n * sizeof(Ins));
	return d + n;
}

void *
valloc(ulong len, size_t esz)
{
	ulong cap;
	Vec *v;

	v = alloc(len * esz + sizeof(Vec));
	v->mag = VMag;
	for (cap=VMin; cap<len; cap*=2)
		;
	v->cap = cap;
	v->esz = esz;
	return v + 1;
}

void
vgrow(void *vp, ulong len)
{
	Vec *v;
	void *v1;

	v = *(Vec **)vp - 1;
	assert(v+1 && v->mag == VMag);
	if (v->cap >= len)
		return;
	v1 = valloc(len, v->esz);
	memcpy(v1, v+1, v->cap * v->esz);
	free(v);
	*(Vec **)vp = v1;
}

Ref
newtmp(char *prfx, Fn *fn)
{
	static int n;
	int t;

	t = fn->ntmp++;
	vgrow(&fn->tmp, fn->ntmp);
	sprintf(fn->tmp[t].name, "%s%d", prfx, ++n);
	fn->tmp[t].spill = -1;
	return TMP(t);
}

Ref
getcon(int64_t val, Fn *fn)
{
	int c;

	for (c=0; c<fn->ncon; c++)
		if (fn->con[c].type == CNum && fn->con[c].val == val)
			return CON(c);
	fn->ncon++;
	vgrow(&fn->con, fn->ncon);
	fn->con[c] = (Con){.type = CNum, .val = val};
	return CON(c);
}