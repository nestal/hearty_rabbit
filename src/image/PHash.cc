/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 5/13/18.
//

#include "PHash.hh"
#include "util/MMap.hh"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <boost/endian/buffers.hpp>
#include <bitset>

namespace hrb {
namespace {

// copy from https://github.com/opencv/opencv_contrib/blob/master/modules/img_hash/src/phash.cpp
class PHashImpl
{
public:
    void compute(cv::InputArray inputArr, cv::OutputArray outputArr)
    {
        cv::Mat const input = inputArr.getMat();
        CV_Assert(input.type() == CV_8UC4 ||
                  input.type() == CV_8UC3 ||
                  input.type() == CV_8U);

        cv::resize(input, resizeImg, cv::Size(32,32), 0, 0, cv::INTER_LINEAR);
        if(input.type() == CV_8UC3)
        {
            cv::cvtColor(resizeImg, grayImg, CV_BGR2GRAY);
        }
        else if(input.type() == CV_8UC4)
        {
            cv::cvtColor(resizeImg, grayImg, CV_BGRA2GRAY);
        }
        else
        {
            grayImg = resizeImg;
        }

        grayImg.convertTo(grayFImg, CV_32F);
        cv::dct(grayFImg, dctImg);
        dctImg(cv::Rect(0, 0, 8, 8)).copyTo(topLeftDCT);
        topLeftDCT.at<float>(0, 0) = 0;
        float const imgMean = static_cast<float>(cv::mean(topLeftDCT)[0]);

        cv::compare(topLeftDCT, imgMean, bitsImg, cv::CMP_GT);
        bitsImg /= 255;
        outputArr.create(1, 8, CV_8U);
        cv::Mat hash = outputArr.getMat();
        uchar *hash_ptr = hash.ptr<uchar>(0);
        uchar const *bits_ptr = bitsImg.ptr<uchar>(0);
        std::bitset<8> bits;
        for(size_t i = 0, j = 0; i != bitsImg.total(); ++j)
        {
            for(size_t k = 0; k != 8; ++k)
            {
                //avoid warning C4800, casting do not work
                bits[k] = bits_ptr[i++] != 0;
            }
            hash_ptr[j] = static_cast<uchar>(bits.to_ulong());
        }
    }

    static double compare(cv::InputArray hashOne, cv::InputArray hashTwo)
    {
        return cv::norm(hashOne, hashTwo, cv::NORM_HAMMING);
    }

private:
    cv::Mat bitsImg;
    cv::Mat dctImg;
    cv::Mat grayFImg;
    cv::Mat grayImg;
    cv::Mat resizeImg;
    cv::Mat topLeftDCT;
};

} // end of local namespace

PHash phash(void *buf, std::size_t size)
{
	if (size > static_cast<decltype(size)>(std::numeric_limits<int>::max()))
		throw std::out_of_range("buffer too big");

	auto input = imdecode(cv::Mat{1, static_cast<int>(size), CV_8U, buf}, cv::IMREAD_GRAYSCALE);
	return input.data ? phash(input) : PHash{};
}

PHash phash(BufferView image)
{
	auto input = imdecode(std::vector<unsigned char>(image.begin(), image.end()), cv::IMREAD_GRAYSCALE);
	return input.data ? phash(input) : PHash{};
}

PHash phash(const cv::Mat& input)
{
	PHashImpl hash;

	cv::Mat out;
	hash.compute(input, out);
	assert(out.rows == 1);
	assert(out.cols == 8);

	return PHash{out};
}

PHash phash(fs::path file)
{
	std::error_code err{};
	auto mmap = MMap::open(file, err);
	return err ? PHash{} : phash(mmap.buffer());
}

PHash::PHash(const cv::_OutputArray& arr)
{
	auto out = arr.getMat();

	// convert to little endian
	boost::endian::little_uint64_buf_t lt_hash{};
	assert(out.cols == sizeof(lt_hash));
	std::memcpy(&lt_hash, out.ptr<unsigned char>(0), sizeof(lt_hash));

	m_hash = lt_hash.value();
}

double PHash::compare(const PHash& other) const
{
	auto hash1 = m_hash;
	auto hash2 = other.m_hash;

	return cv::norm(
		cv::Mat{1, sizeof(hash1), CV_8U, reinterpret_cast<void*>(&hash1)},
		cv::Mat{1, sizeof(hash2), CV_8U, reinterpret_cast<void*>(&hash2)},
		cv::NORM_HAMMING
	);
}

} // end of namespace hrb
