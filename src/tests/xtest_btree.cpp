/*
 * test_btree.cpp
 *
 *  Created on: 9. 4. 2016
 *      Author: ondra
 */


#include "../lightspeed/base/framework/testapp.h"
#include "../lightspeed/base/text/textstream.tcc"
#include "../lightspeed/base/containers/btree.h"
#include "../lightspeed/base/exceptions/errorMessageException.h"

namespace LightSpeed {
template class BTree<int>;
}

namespace LightSpeedTest {


using namespace LightSpeed;

class NonPodObject: public ComparableLess<NonPodObject> {
public:
	int number;
	int magix;
	NonPodObject *left;
	NonPodObject *right;

	static NonPodObject central;

	NonPodObject(int number):number(number) {
		if (magix == 0x123456)
			throw ErrorMessageException(THISLOCATION,"Override error while creation");
		magix = 0x123456;
		right = central.right;
		central.right = this;
		left = &central;
		right->left = this;
	}

	NonPodObject(int number, bool central):number(number),left(this),right(this) {
		magix = 0x123456;
	}

	~NonPodObject() {
		if (magix != 0x123456)
			throw ErrorMessageException(THISLOCATION,"Corrupted item");
		magix = 0;
		left->right = right;
		right->left = left;
		right = 0;
		left = 0;
	}

	NonPodObject(const NonPodObject &other):number(other.number) {
		if (magix == 0x123456)
			throw ErrorMessageException(THISLOCATION,"Override error while copying");
		magix = 0x123456;
		right = central.right;
		central.right = this;
		left = &central;
		right->left = this;
	}


	bool lessThan(const NonPodObject &other) const {
		return number < other.number;
	}

};

NonPodObject NonPodObject::central(0,true);

static void btreeOriginalTest(PrintTextA &a)
  {
	{
	  BTree<NonPodObject> tree(10);
	  int p=10;
	  tree.Add(p);
	  int i;
	//  DWORD tck=GetTickCount();
	  for (i=0;i<100000;i++)
		tree.Add(i);
	//  printf("%d\n",GetTickCount()-tck);
	//  tck=GetTickCount();
	  for (i=0;i<100000;i++)
		{
		NonPodObject *q=tree.Find(i);
		if (q->number != i) {
			a("%1 != %2") << q->number << i;
			return;
		}
		}
	//  printf("%d\n",GetTickCount()-tck);
	  NonPodObject *l=tree.Largest();
	  a("%1 ") << l->number;
	  NonPodObject *s=tree.Smallest();
	  a("%1 ") << s->number;
	  int cnt=tree.Size();
	  a("%1 ") << cnt;

	  for (i=0;i<100;i++)
		tree.Remove(i);

	  BTreeIterator<NonPodObject> itr(tree);
	  itr.BeginFrom(p);
	  for (i=0;i<100;i++)
		{
		s=itr.Next();
		if (s==NULL) break;
		a("%1 ") << s->number;
		}
	}
	a("\n");
  NonPodObject *x = NonPodObject::central.right;
  while (x != &NonPodObject::central) {
	  a("%1") << x->number;
	  x = x->right;
  }
  }



defineTest test_btreeOriginal("btree.originalTest","99999 0 100000 100 101 102 103 104 105 106 107 108 109 110 111 112 113 114 115 116 117 118 119 120 121 122 123 124 125 126 127 128 129 130 131 132 133 134 135 136 137 138 139 140 141 142 143 144 145 146 147 148 149 150 151 152 153 154 155 156 157 158 159 160 161 162 163 164 165 166 167 168 169 170 171 172 173 174 175 176 177 178 179 180 181 182 183 184 185 186 187 188 189 190 191 192 193 194 195 196 197 198 199 \n",&btreeOriginalTest);

}


