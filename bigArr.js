// We don't have access to pointers so we must use Array
// instead of BigUint64Array. I don't know if `new Array (256)`
// is better than `return []`, but at least it communicates the intent
//
function mk_chunk ()
{
  return new Array (256);
}

// There is no uint64 type and Number is too small, so
// we are forced to use BigInt for indicies:
//
//   arr = new BigArr;
//   arr.set (1234n, "foo"); // 1234n, not 1234
//
class BigArr
{
  arr = mk_chunk ();

  index_chunk (ix0)
  {
    let buf = new ArrayBuffer(8);
    new BigUint64Array(buf)[0] = ix0;
    new Uint8Array(buf).reverse();

    let ix  = new BigUint64Array(buf)[0];
    let cur = this.arr;

    for (let i = 0; i < 7; i++) {
      const lx = BigInt.asUintN(8, ix);
      if (cur[lx] === undefined) cur[lx] = mk_chunk();
      cur = cur[lx];
      ix >>= 8n;
    }

    return { arr: cur, ix: BigInt.asUintN(8, ix) };
  }

  get (ix)
  {
    const res = this.index_chunk(ix);
    return res.arr[res.ix];
  }

  set (ix, val)
  {
    const res = this.index_chunk(ix);
    return res.arr[res.ix] = val;
  }
}

