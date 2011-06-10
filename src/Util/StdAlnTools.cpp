///-----------------------------------------------
// Copyright 2011 Wellcome Trust Sanger Institute
// Written by Jared Simpson (js18@sanger.ac.uk)
// Released under the GPL
//-----------------------------------------------
//
// StdAlnTools - Collection of wrappers around the
// stdaln dynamic programming alignment functions
#include <assert.h>
#include "StdAlnTools.h"
#include "stdaln.h"
#include "Alphabet.h"

void StdAlnTools::printGlobalAlignment(const std::string& target, const std::string& query)
{
    // Set up global alignment parameters and data structures
    GlobalAlnParams params;
	size_t max_target = StdAlnTools::calculateMaxTargetLength(query.size(), params);
    int max_path_length = max_target + query.size();
    path_t* path = (path_t*)calloc(max_path_length, sizeof(path_t));    

    AlnParam par;
    int matrix[25];
    StdAlnTools::setAlnParam(par, matrix, params);

    // Make a packed version of the query and target
    uint8_t* pQueryT = createPacked(query);
    uint8_t* pTargetT = createPacked(target);
    int path_len = 0;
    int score = aln_global_core(pTargetT, target.size(), pQueryT, query.size(), &par, path, &path_len);

    assert(path_len <= max_path_length);
    std::string paddedTarget, paddedQuery, paddedMatch;
    makePaddedStrings(target, query, path, path_len, paddedTarget, paddedQuery, paddedMatch);
    printPaddedStrings(paddedTarget, paddedQuery, paddedMatch);
    std::cout << "Global alignment score: " << score << "\n";
    delete [] pQueryT;
    delete [] pTargetT;
    free(path);
}

// Convert a std::string into the stdaln required packed format.
// This function allocates memory which the caller must free
uint8_t* StdAlnTools::createPacked(const std::string& s, size_t start, size_t length)
{
    if(length == std::string::npos)
        length = s.size();
    assert(length <= s.size());

    uint8_t* pBuffer = new uint8_t[length];
    for(size_t j = 0; j < length; ++j)
        pBuffer[j] = DNA_ALPHABET::getBaseRank(s[start + j]);
    return pBuffer;
}


// Calculate the maximum target length for a query of length ql
size_t StdAlnTools::calculateMaxTargetLength(int ql, const GlobalAlnParams& params)
{
    size_t mt = ((ql + 1) / 2 * params.match + params.gap_ext) / params.gap_ext + ql;
    return mt;
}

//
void StdAlnTools::setAlnParam(AlnParam& par, int matrix[25], const GlobalAlnParams& params)
{
    par.matrix = matrix;

    // Set matrix
	for (size_t i = 0; i < 25; ++i) par.matrix[i] = -params.mismatch;
	for (size_t i = 0; i < 4; ++i) par.matrix[i*5+i] = params.match; 
	par.gap_open = params.gap_open;
    par.gap_ext = params.gap_ext;
    par.gap_end = params.gap_ext;
	par.row = 5; 
    par.band_width = params.bandwidth;
}

// Convert a dynamic programming path to a set of padded strings
// Algorithm ported from stdaln.c:aln_stdaln_aux
void StdAlnTools::makePaddedStrings(const std::string& s1, const std::string& s2, path_t* path, int path_len, 
                                    std::string& out1, std::string& out2, std::string& outm)
{
    path_t* p = path + path_len;

    out1.resize(path_len + 1, 'A');
    out2.resize(path_len + 1, 'A');
    outm.resize(path_len + 1, 'A');

    for (int l = 0; p >= path; --p, ++l) {
        switch (p->ctype) 
        {
            case FROM_M: 
                out1[l] = s1[p->i]; 
                out2[l] = s2[p->j];
                outm[l] = (s1[p->i] == s2[p->j] /*&& s1[p->i] != ap->row*/)? '|' : ' ';
                break;
            case FROM_I: 
                out1[l] = '-'; 
                out2[l] = s2[p->j]; 
                outm[l] = ' '; 
                break;
            case FROM_D: 
                out1[l] = s1[p->i]; 
                out2[l] = '-'; 
                outm[l] = ' '; 
                break;
        }
    }    
}

// 
void StdAlnTools::printPaddedStrings(const std::string& s1, const std::string& s2, const std::string& m, int colSize)
{
    assert(s1.size() == s2.size() && s1.size() == m.size());
    size_t len = s1.size();
    for(size_t l = 0; l < len; l += colSize)
    {
        int diff = len - l;
        int stop = diff < colSize ? diff : colSize;
        printf("1\t%s\n", s1.substr(l, stop).c_str());   
        printf("2\t%s\n", s2.substr(l, stop).c_str());   
        printf("m\t%s\n\n", m.substr(l, stop).c_str());
    }
}
