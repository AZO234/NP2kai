#include	"compiler.h"


BOOL rect_in(const RECT_T *rect, int x, int y) {

	if ((rect) &&
		(rect->left <= x) && (rect->right > x) &&
		(rect->top <= y) && (rect->bottom > y)) {
		return(TRUE);
	}
	return(FALSE);
}

int rect_num(const RECT_T *rect, int cnt, int x, int y) {

	int		i;

	if (rect) {
		for (i=0; i<cnt; i++, rect++) {
			if ((rect->left <= x) && (x < rect->right) &&
				(rect->top <= y) && (y < rect->bottom)) {
				return(i);
			}
		}
	}
	return(-1);
}

BOOL rect_isoverlap(const RECT_T *r1, const RECT_T *r2) {

	if ((r1->left >= r2->right) ||
		(r1->right <= r2->left) ||
		(r1->top >= r2->bottom) ||
		(r1->bottom <= r2->top)) {
		return(FALSE);
	}
	return(TRUE);
}

void rect_enumout(const RECT_T *tag, const RECT_T *base,
				void *arg, void (*outcb)(void *arg, const RECT_T *rect)) {

	RECT_T	rect;

	if ((tag != NULL) && (base != NULL) && (outcb != NULL)) {
		// base.top -> tag.top
		rect.top = base->top;
		rect.bottom = min(tag->top, base->bottom);
		if (rect.top < rect.bottom) {
			rect.left = base->left;
			rect.right = base->right;
			outcb(arg, &rect);
				rect.top = rect.bottom;
		}

		// -> tag.bottom
		rect.bottom = min(tag->bottom, base->bottom);
		if (rect.top < rect.bottom) {
			rect.left = base->left;
			rect.right = min(tag->left, base->right);
			if (rect.left < rect.right) {
				outcb(arg, &rect);
			}
			rect.left = max(tag->right, base->left);
			rect.right = base->right;
			if (rect.left < rect.right) {
				outcb(arg, &rect);
			}
			rect.top = rect.bottom;
		}

		// -> base.bottom
		rect.bottom = base->bottom;
		if (rect.top < rect.bottom) {
			rect.left = base->left;
			rect.right = base->right;
			outcb(arg, &rect);
		}
	}
}

void rect_add(RECT_T *dst, const RECT_T *src) {

	if (dst->left > src->left) {
		dst->left = src->left;
	}
	if (dst->top > src->top) {
		dst->top = src->top;
	}
	if (dst->right < src->right) {
		dst->right = src->right;
	}
	if (dst->bottom < src->bottom) {
		dst->bottom = src->bottom;
	}
}

void unionrect_rst(UNIRECT *unirct) {

	if (unirct) {
		unirct->type = 0;
	}
}

void unionrect_add(UNIRECT *unirct, const RECT_T *rct) {

	int		type;
	RECT_T	*r;

	if (unirct == NULL) {
		goto ura_end;
	}
	type = unirct->type;
	if (type < 0) {
		goto ura_end;
	}
	if (rct == NULL) {
		type = -1;
	}
	else {
		r = &unirct->r;
		if (type++ == 0) {
			unirct->r = *rct;
		}
		else {
			if (r->left > rct->left) {
				r->left = rct->left;
			}
			if (r->top > rct->top) {
				r->top = rct->top;
			}
			if (r->right < rct->right) {
				r->right = rct->right;
			}
			if (r->bottom < rct->bottom) {
				r->bottom = rct->bottom;
			}
		}
	}
	unirct->type = type;

ura_end:
	return;
}

const RECT_T *unionrect_get(const UNIRECT *unirct) {

	if ((unirct) && (unirct->type > 0)) {
		return(&unirct->r);
	}
	else {
		return(NULL);
	}
}

