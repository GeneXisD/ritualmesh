/*
 * 'OpenSSL for Ruby' project
 * Copyright (C) 2001-2002  Michal Rokos <m.rokos@sh.cvut.cz>
 * All rights reserved.
 */
/*
 * This program is licensed under the same licence as Ruby.
 * (See the file 'LICENCE'.)
 */
#include "ossl.h"

#define GetDigest(obj, ctx) do { \
    TypedData_Get_Struct((obj), EVP_MD_CTX, &ossl_digest_type, (ctx)); \
    if (!(ctx)) { \
	ossl_raise(rb_eRuntimeError, "Digest CTX wasn't initialized!"); \
    } \
} while (0)

/*
 * Classes
 */
VALUE cDigest;
VALUE eDigestError;

static VALUE ossl_digest_alloc(VALUE klass);

static void
ossl_digest_free(void *ctx)
{
    EVP_MD_CTX_destroy(ctx);
}

static const rb_data_type_t ossl_digest_type = {
    "OpenSSL/Digest",
    {
	0, ossl_digest_free,
    },
    0, 0, RUBY_TYPED_FREE_IMMEDIATELY,
};

/*
 * Public
 */
const EVP_MD *
ossl_evp_get_digestbyname(VALUE obj)
{
    const EVP_MD *md;
    ASN1_OBJECT *oid = NULL;

    if (RB_TYPE_P(obj, T_STRING)) {
    	const char *name = StringValueCStr(obj);

	md = EVP_get_digestbyname(name);
	if (!md) {
	    oid = OBJ_txt2obj(name, 0);
	    md = EVP_get_digestbyobj(oid);
	    ASN1_OBJECT_free(oid);
	}
	if(!md)
            ossl_raise(rb_eRuntimeError, "Unsupported digest algorithm (%"PRIsVALUE").", obj);
    } else {
        EVP_MD_CTX *ctx;

        GetDigest(obj, ctx);

        md = EVP_MD_CTX_get0_md(ctx);
    }

    return md;
}

VALUE
ossl_digest_new(const EVP_MD *md)
{
    VALUE ret;
    EVP_MD_CTX *ctx;

    ret = ossl_digest_alloc(cDigest);
    ctx = EVP_MD_CTX_new();
    if (!ctx)
	ossl_raise(eDigestError, "EVP_MD_CTX_new");
    RTYPEDDATA_DATA(ret) = ctx;

    if (!EVP_DigestInit_ex(ctx, md, NULL))
	ossl_raise(eDigestError, "Digest initialization failed");

    return ret;
}

/*
 * Private
 */
static VALUE
ossl_digest_alloc(VALUE klass)
{
    return TypedData_Wrap_Struct(klass, &ossl_digest_type, 0);
}

VALUE ossl_digest_update(VALUE, VALUE);

/*
 *  call-seq:
 *     Digest.new(string [, data]) -> Digest
 *
 * Creates a Digest instance based on _string_, which is either the ln
 * (long name) or sn (short name) of a supported digest algorithm.
 *
 * If _data_ (a String) is given, it is used as the initial input to the
 * Digest instance, i.e.
 *
 *   digest = OpenSSL::Digest.new('sha256', 'digestdata')
 *
 * is equivalent to
 *
 *   digest = OpenSSL::Digest.new('sha256')
 *   digest.update('digestdata')
 */
static VALUE
ossl_digest_initialize(int argc, VALUE *argv, VALUE self)
{
    EVP_MD_CTX *ctx;
    const EVP_MD *md;
    VALUE type, data;

    rb_scan_args(argc, argv, "11", &type, &data);
    md = ossl_evp_get_digestbyname(type);
    if (!NIL_P(data)) StringValue(data);

    TypedData_Get_Struct(self, EVP_MD_CTX, &ossl_digest_type, ctx);
    if (!ctx) {
	RTYPEDDATA_DATA(self) = ctx = EVP_MD_CTX_new();
	if (!ctx)
	    ossl_raise(eDigestError, "EVP_MD_CTX_new");
    }

    if (!EVP_DigestInit_ex(ctx, md, NULL))
	ossl_raise(eDigestError, "Digest initialization failed");

    if (!NIL_P(data)) return ossl_digest_update(self, data);
    return self;
}

static VALUE
ossl_digest_copy(VALUE self, VALUE other)
{
    EVP_MD_CTX *ctx1, *ctx2;

    rb_check_frozen(self);
    if (self == other) return self;

    TypedData_Get_Struct(self, EVP_MD_CTX, &ossl_digest_type, ctx1);
    if (!ctx1) {
	RTYPEDDATA_DATA(self) = ctx1 = EVP_MD_CTX_new();
	if (!ctx1)
	    ossl_raise(eDigestError, "EVP_MD_CTX_new");
    }
    GetDigest(other, ctx2);

    if (!EVP_MD_CTX_copy(ctx1, ctx2)) {
	ossl_raise(eDigestError, NULL);
    }
    return self;
}

/*
 *  call-seq:
 *     digest.reset -> self
 *
 * Resets the Digest in the sense that any Digest#update that has been
 * performed is abandoned and the Digest is set to its initial state again.
 *
 */
static VALUE
ossl_digest_reset(VALUE self)
{
    EVP_MD_CTX *ctx;

    GetDigest(self, ctx);
    if (EVP_DigestInit_ex(ctx, EVP_MD_CTX_get0_md(ctx), NULL) != 1) {
	ossl_raise(eDigestError, "Digest initialization failed.");
    }

    return self;
}

/*
 *  call-seq:
 *     digest.update(string) -> aString
 *
 * Not every message digest can be computed in one single pass. If a message
 * digest is to be computed from several subsequent sources, then each may
 * be passed individually to the Digest instance.
 *
 * === Example
 *   digest = OpenSSL::Digest.new('SHA256')
 *   digest.update('First input')
 *   digest << 'Second input' # equivalent to digest.update('Second input')
 *   result = digest.digest
 *
 */
VALUE
ossl_digest_update(VALUE self, VALUE data)
{
    EVP_MD_CTX *ctx;

    StringValue(data);
    GetDigest(self, ctx);

    if (!EVP_DigestUpdate(ctx, RSTRING_PTR(data), RSTRING_LEN(data)))
	ossl_raise(eDigestError, "EVP_DigestUpdate");

    return self;
}

/*
 *  call-seq:
 *      digest.finish -> aString
 *
 */
static VALUE
ossl_digest_finish(int argc, VALUE *argv, VALUE self)
{
    EVP_MD_CTX *ctx;
    VALUE str;
    int out_len;

    GetDigest(self, ctx);
    rb_scan_args(argc, argv, "01", &str);
    out_len = EVP_MD_CTX_size(ctx);

    if (NIL_P(str)) {
        str = rb_str_new(NULL, out_len);
    } else {
        StringValue(str);
        rb_str_modify(str);
        rb_str_resize(str, out_len);
    }

    if (!EVP_DigestFinal_ex(ctx, (unsigned char *)RSTRING_PTR(str), NULL))
	ossl_raise(eDigestError, "EVP_DigestFinal_ex");

    return str;
}

/*
 *  call-seq:
 *      digest.name -> string
 *
 * Returns the sn of this Digest algorithm.
 *
 * === Example
 *   digest = OpenSSL::Digest.new('SHA512')
 *   puts digest.name # => SHA512
 *
 */
static VALUE
ossl_digest_name(VALUE self)
{
    EVP_MD_CTX *ctx;

    GetDigest(self, ctx);

    return rb_str_new_cstr(EVP_MD_name(EVP_MD_CTX_get0_md(ctx)));
}

/*
 *  call-seq:
 *      digest.digest_length -> integer
 *
 * Returns the output size of the digest, i.e. the length in bytes of the
 * final message digest result.
 *
 * === Example
 *   digest = OpenSSL::Digest.new('SHA1')
 *   puts digest.digest_length # => 20
 *
 */
static VALUE
ossl_digest_size(VALUE self)
{
    EVP_MD_CTX *ctx;

    GetDigest(self, ctx);

    return INT2NUM(EVP_MD_CTX_size(ctx));
}

/*
 *  call-seq:
 *      digest.block_length -> integer
 *
 * Returns the block length of the digest algorithm, i.e. the length in bytes
 * of an individual block. Most modern algorithms partition a message to be
 * digested into a sequence of fix-sized blocks that are processed
 * consecutively.
 *
 * === Example
 *   digest = OpenSSL::Digest.new('SHA1')
 *   puts digest.block_length # => 64
 */
static VALUE
ossl_digest_block_length(VALUE self)
{
    EVP_MD_CTX *ctx;

    GetDigest(self, ctx);

    return INT2NUM(EVP_MD_CTX_block_size(ctx));
}

/*
 * INIT
 */
void
Init_ossl_digest(void)
{
#if 0
    mOSSL = rb_define_module("OpenSSL");
    eOSSLError = rb_define_class_under(mOSSL, "OpenSSLError", rb_eStandardError);
#endif

    /* Document-class: OpenSSL::Digest
     *
     * OpenSSL::Digest allows you to compute message digests (sometimes
     * interchangeably called "hashes") of arbitrary data that are
     * cryptographically secure, i.e. a Digest implements a secure one-way
     * function.
     *
     * One-way functions offer some useful properties. E.g. given two
     * distinct inputs the probability that both yield the same output
     * is highly unlikely. Combined with the fact that every message digest
     * algorithm has a fixed-length output of just a few bytes, digests are
     * often used to create unique identifiers for arbitrary data. A common
     * example is the creation of a unique id for binary documents that are
     * stored in a database.
     *
     * Another useful characteristic of one-way functions (and thus the name)
     * is that given a digest there is no indication about the original
     * data that produced it, i.e. the only way to identify the original input
     * is to "brute-force" through every possible combination of inputs.
     *
     * These characteristics make one-way functions also ideal companions
     * for public key signature algorithms: instead of signing an entire
     * document, first a hash of the document is produced with a considerably
     * faster message digest algorithm and only the few bytes of its output
     * need to be signed using the slower public key algorithm. To validate
     * the integrity of a signed document, it suffices to re-compute the hash
     * and verify that it is equal to that in the signature.
     *
     * You can get a list of all digest algorithms supported on your system by
     * running this command in your terminal:
     *
     *   openssl list -digest-algorithms
     *
     * Among the OpenSSL 1.1.1 supported message digest algorithms are:
     * * SHA224, SHA256, SHA384, SHA512, SHA512-224 and SHA512-256
     * * SHA3-224, SHA3-256, SHA3-384 and SHA3-512
     * * BLAKE2s256 and BLAKE2b512
     *
     * Each of these algorithms can be instantiated using the name:
     *
     *   digest = OpenSSL::Digest.new('SHA256')
     *
     * "Breaking" a message digest algorithm means defying its one-way
     * function characteristics, i.e. producing a collision or finding a way
     * to get to the original data by means that are more efficient than
     * brute-forcing etc. Most of the supported digest algorithms can be
     * considered broken in this sense, even the very popular MD5 and SHA1
     * algorithms. Should security be your highest concern, then you should
     * probably rely on SHA224, SHA256, SHA384 or SHA512.
     *
     * === Hashing a file
     *
     *   data = File.binread('document')
     *   sha256 = OpenSSL::Digest.new('SHA256')
     *   digest = sha256.digest(data)
     *
     * === Hashing several pieces of data at once
     *
     *   data1 = File.binread('file1')
     *   data2 = File.binread('file2')
     *   data3 = File.binread('file3')
     *   sha256 = OpenSSL::Digest.new('SHA256')
     *   sha256 << data1
     *   sha256 << data2
     *   sha256 << data3
     *   digest = sha256.digest
     *
     * === Reuse a Digest instance
     *
     *   data1 = File.binread('file1')
     *   sha256 = OpenSSL::Digest.new('SHA256')
     *   digest1 = sha256.digest(data1)
     *
     *   data2 = File.binread('file2')
     *   sha256.reset
     *   digest2 = sha256.digest(data2)
     *
     */

    /*
     * Digest::Class is defined by the digest library. rb_require() cannot be
     * used here because it bypasses RubyGems.
     */
    rb_funcall(Qnil, rb_intern_const("require"), 1, rb_str_new_cstr("digest"));
    cDigest = rb_define_class_under(mOSSL, "Digest", rb_path2class("Digest::Class"));
    /* Document-class: OpenSSL::Digest::DigestError
     *
     * Generic Exception class that is raised if an error occurs during a
     * Digest operation.
     */
    eDigestError = rb_define_class_under(cDigest, "DigestError", eOSSLError);

    rb_define_alloc_func(cDigest, ossl_digest_alloc);

    rb_define_method(cDigest, "initialize", ossl_digest_initialize, -1);
    rb_define_method(cDigest, "initialize_copy", ossl_digest_copy, 1);
    rb_define_method(cDigest, "reset", ossl_digest_reset, 0);
    rb_define_method(cDigest, "update", ossl_digest_update, 1);
    rb_define_alias(cDigest, "<<", "update");
    rb_define_private_method(cDigest, "finish", ossl_digest_finish, -1);
    rb_define_method(cDigest, "digest_length", ossl_digest_size, 0);
    rb_define_method(cDigest, "block_length", ossl_digest_block_length, 0);

    rb_define_method(cDigest, "name", ossl_digest_name, 0);
}
