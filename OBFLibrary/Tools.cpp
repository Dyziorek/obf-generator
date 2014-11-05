#include "stdafx.h"

#include <cassert>
#include <limits>
#include <cmath>
#include <codecvt>
#include "MapUtils.h"
#include <boost\regex.hpp>
#include "Tools.h"


#pragma push_macro("max")
#undef max
#pragma push_macro("min")
#undef min


bool Tools::extractFirstNumberPosition( const std::string& value, int& first, int& last, bool allowSigned, bool allowDot )
{
    first = -1;
    last = -1;
    int curPos = 0;
    for(auto itChr = value.cbegin(); itChr != value.cend() && (first == -1 || last == -1); ++itChr, curPos++)
    {
        auto chr = *itChr;
        if(first == -1 && isdigit(chr))
            first = curPos;
        if(last == -1 && first != -1 && !isdigit(chr) && ((allowDot && chr != '.') || !allowDot))
            last = curPos - 1;
    }
    if(first >= 1 && allowSigned && value[first - 1] == '-')
        first -= 1;
    if(first != -1 && last == -1)
        last = value.length() - 1;
    return first != -1;
}

double Tools::parseSpeed( const std::string& value, double defValue, bool* wasParsed/* = nullptr*/ )
{
    if(value == "none")
    {
        if(wasParsed)
            *wasParsed = true;
        return 40;
    }

    int first, last;
    if(!extractFirstNumberPosition(value, first, last, false, true))
    {
        if(wasParsed)
            *wasParsed = false;
        return defValue;
    }
    bool ok = true;
	double result;
	try
	{
		result = boost::lexical_cast<double>(value.substr(first, last - first + 1));
	}
	catch (boost::bad_lexical_cast& blex)
	{
		ok = false;
	}
    if(wasParsed)
        *wasParsed = ok;
    if(!ok)
        return defValue;

    result /= 3.6;
    if(value.find("mph") != std::string::npos)
        result *= 1.6;
    return result;
}

double Tools::parseLength( const std::string& value, double defValue, bool* wasParsed/* = nullptr*/ )
{
    int first, last;
    if(!extractFirstNumberPosition(value, first, last, false, true))
        return defValue;

    bool ok = true;
	double result;
	try
	{
		result = boost::lexical_cast<double>(value.substr(first, last - first + 1));
	}
	catch (boost::bad_lexical_cast& blex)
	{
		ok = false;
	}
    if(!ok)
        return defValue;
    if(value.find("ft") != std::string::npos || value.find('"') != std::string::npos)
        result *= 0.3048;
    if(value.find('\'') != std::string::npos)
    {
        auto inchesSubstr = value.substr(value.find('"') + 1);
        if(!extractFirstNumberPosition(inchesSubstr, first, last, false, true))
            return defValue;
        bool ok = true;
		double inches;
		try
		{
			inches = boost::lexical_cast<double>(inchesSubstr.substr(first, last - first + 1));
		}
		catch (boost::bad_lexical_cast& blex)
		{
			ok = false;
		}
    
        if(ok)
            result += inches * 0.0254;        
    }
    return result;
}

double Tools::parseWeight( const std::string& value, double defValue, bool* wasParsed/* = nullptr*/ )
{
    int first, last;
    if(wasParsed)
        *wasParsed = false;
    if(!extractFirstNumberPosition(value, first, last, false, true))
        return defValue;
    
	bool ok = true;
	double result;
	try
	{
		result = boost::lexical_cast<double>(value.substr(first, last - first + 1));
	}
	catch (boost::bad_lexical_cast& blex)
	{
		ok = false;
	}
    if(!ok)
        return defValue;

    if(wasParsed)
        *wasParsed = true;
    if(value.find("lbs") != std::string::npos)
        result = (result * 0.4535) / 1000.0; // lbs -> kg -> ton
    return result;
}

int Tools::parseArbitraryInt( const std::string& value, int defValue, bool* wasParsed/* = nullptr*/ )
{
    int first, last;
    if(wasParsed)
        *wasParsed = false;
    if(!extractFirstNumberPosition(value, first, last, true, false))
        return defValue;
    bool ok = true;
	int result;
	try
	{
		result = boost::lexical_cast<int>(value.substr(first, last - first + 1));
	}
	catch (boost::bad_lexical_cast& blex)
	{
		ok = false;
	}
    if(!ok)
        return defValue;

    if(wasParsed)
        *wasParsed = true;
    return result;
}

long Tools::parseArbitraryLong( const std::string& value, long defValue, bool* wasParsed/* = nullptr*/ )
{
    int first, last;
    if(wasParsed)
        *wasParsed = false;
    if(!extractFirstNumberPosition(value, first, last, true, false))
        return defValue;
   bool ok = true;
	long result;
	try
	{
		result = boost::lexical_cast<long>(value.substr(first, last - first + 1));
	}
	catch (boost::bad_lexical_cast& blex)
	{
		ok = false;
	}
    if(!ok)
        return defValue;

    if(wasParsed)
        *wasParsed = true;
    return result;
}

unsigned int Tools::parseArbitraryUInt( const std::string& value, unsigned int defValue, bool* wasParsed/* = nullptr*/ )
{
    int first, last;
    if(wasParsed)
        *wasParsed = false;
    if(!extractFirstNumberPosition(value, first, last, false, false))
        return defValue;
    bool ok = true;
	unsigned int result;
	try
	{
		result = boost::lexical_cast<unsigned int>(value.substr(first, last - first + 1));
	}
	catch (boost::bad_lexical_cast& blex)
	{
		ok = false;
	}
    if(!ok)
        return defValue;

    if(wasParsed)
        *wasParsed = true;
    return result;
}

unsigned long Tools::parseArbitraryULong( const std::string& value, unsigned long defValue, bool* wasParsed/* = nullptr*/ )
{
    int first, last;
    if(wasParsed)
        *wasParsed = false;
    if(!extractFirstNumberPosition(value, first, last, false, false))
        return defValue;
    bool ok = true;
	unsigned long result;
	try
	{
		result = boost::lexical_cast<unsigned long>(value.substr(first, last - first + 1));
	}
	catch (boost::bad_lexical_cast& blex)
	{
		ok = false;
	}
    if(!ok)
        return defValue;

    if(wasParsed)
        *wasParsed = true;
    return result;
}

float Tools::parseArbitraryFloat( const std::string& value, float defValue, bool* wasParsed /*= nullptr*/ )
{
    int first, last;
    if(wasParsed)
        *wasParsed = false;
    if(!extractFirstNumberPosition(value, first, last, true, true))
        return defValue;

    bool ok = true;
	float result;
	try
	{
		result = boost::lexical_cast<float>(value.substr(first, last - first + 1));
	}
	catch (boost::bad_lexical_cast& blex)
	{
		ok = false;
	}
    if(!ok)
        return defValue;

    if(wasParsed)
        *wasParsed = true;
    return result;
}

 bool Tools::parseArbitraryBool( const std::string& value, bool defValue, bool* wasParsed /*= nullptr*/ )
{
    if(wasParsed)
        *wasParsed = false;

    if(value.empty())
        return defValue;

    auto result = boost::iequals(value, "true");

    if(wasParsed)
        *wasParsed = true;
    return result;
}

  int Tools::javaDoubleCompare( double l, double r )
{
    const auto lNaN = _isnan(l);
    const auto rNaN = _isnan(r);
    const auto& li64 = *reinterpret_cast<uint64_t*>(&l);
    const auto& ri64 = *reinterpret_cast<uint64_t*>(&r);
    const auto lPos = (li64 >> 63) == 0;
    const auto rPos = (ri64 >> 63) == 0;
    const auto lZero = (li64 << 1) == 0;
    const auto rZero = (ri64 << 1) == 0;

    // NaN is considered by this method to be equal to itself and greater than all other double values (including +inf).
    if(lNaN && rNaN)
        return 0;
    if(lNaN)
        return +1;
    if(rNaN)
        return -1;

    // 0.0 is considered by this method to be greater than -0.0
    if(lZero && rZero)
    {
        if(lPos && !rPos)
            return -1;
        if(!lPos && rPos)
            return +1;
    }

    // All other cases
    return ceil(l) - ceil(r);
}

 void Tools::findFiles(const boost::filesystem::path& origin, const std::vector<std::string>& masks, std::vector<std::string>& files, const bool recursively)
{
	if (!boost::filesystem::is_directory(origin))
		return;

	std::vector<boost::wregex> wregexes;
	for (auto cmask : masks)
	{
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		std::wstring mask = converter.from_bytes(cmask.c_str());
		boost::replace_all(mask, L"\\", L"\\\\");
		boost::replace_all(mask, L"^", L"\\^");
		boost::replace_all(mask, L".", L"\\.");
		boost::replace_all(mask, L"$", L"\\$");
		boost::replace_all(mask, L"|", L"\\|");
		boost::replace_all(mask, L"(", L"\\(");
		boost::replace_all(mask, L")", L"\\)");
		boost::replace_all(mask, L"[", L"\\[");
		boost::replace_all(mask, L"]", L"\\]");
		boost::replace_all(mask, L"*", L"\\*");
		boost::replace_all(mask, L"+", L"\\+");
		boost::replace_all(mask, L"?", L"\\?");
		boost::replace_all(mask, L"/", L"\\/");
		boost::replace_all(mask, L"\\?", L".");
		boost::replace_all(mask, L"\\*", L".*");
		boost::wregex pattern(mask,  boost::regex::icase);
		wregexes.push_back(pattern);
	}



	
	
	boost::filesystem::directory_iterator itDirectory(origin);
	try
	{
	for(itDirectory; ; itDirectory++)
	{
		boost::filesystem::path partPath = itDirectory->path();

		bool match = false;
		for (boost::wregex regexMask : wregexes)
		{
			if(boost::regex_match(partPath.filename().c_str(), regexMask))
			{
				files.push_back(partPath.string());
				break;
			}
		}

		
	}
	}
	catch(...)
	{

	}


}

 void Tools::scanlineFillPolygon( const unsigned int verticesCount, const pointF* vertices, std::function<void (const pointI&)> fillPoint )
{
    // Find min-max of Y
    float yMinF, yMaxF;
    yMinF = yMaxF = vertices[0].get<1>();
    for(auto idx = 1u; idx < verticesCount; idx++)
    {
        const auto& y = vertices[idx].get<1>();
        if(y > yMaxF)
            yMaxF = y;
        if(y < yMinF)
            yMinF = y;
    }
    const auto rowMin = floor(yMinF);
    const auto rowMax = floor(yMaxF);

    // Build set of edges
    struct Edge
    {
        const pointF* v0;
        int startRow;
        const pointF* v1;
        int endRow;
        float xOrigin;
        float slope;
        int nextRow;
    };
    std::vector<Edge*> edges;
    edges.reserve(verticesCount);
    auto edgeIdx = 0u;
    for(auto idx = 0u, prevIdx = verticesCount - 1; idx < verticesCount; prevIdx = idx++)
    {
        auto v0 = &vertices[prevIdx];
        auto v1 = &vertices[idx];

		if(v0->get<1>() == v1->get<1>())
        {
            // Horizontal edge
            auto edge = new Edge();
            edge->v0 = v0;
            edge->v1 = v1;
            edge->startRow = floor(edge->v0->get<1>());
            edge->endRow = floor(edge->v1->get<1>());
            edge->xOrigin = edge->v0->get<0>();
            edge->slope = 0;
			edge->nextRow = floor(edge->v0->get<1>()) + 1;
            edges.push_back(edge);
            //LogPrintf(LogSeverityLevel::Debug, "Edge %p y(%d %d)(%f %f), next row = %d", edge, edge->startRow, edge->endRow, edge->v0->y, edge->v1->y, edge->nextRow);

            continue;
        }

        const pointF* pLower = nullptr;
        const pointF* pUpper = nullptr;
        if(v0->get<1>() < v1->get<1>())
        {
            // Up-going edge
            pLower = v0;
            pUpper = v1;
        }
        else if(v0->get<1>() > v1->get<1>())
        {
            // Down-going edge
            pLower = v1;
            pUpper = v0;
        }
        
        // Fill edge 
        auto edge = new Edge();
        edge->v0 = pLower;
        edge->v1 = pUpper;
        edge->startRow = floor(edge->v0->get<1>());
        edge->endRow = floor(edge->v1->get<1>());
        edge->slope = (edge->v1->get<0>() - edge->v0->get<0>()) / (edge->v1->get<1>() - edge->v0->get<1>());
        edge->xOrigin = edge->v0->get<0>() - edge->slope * (edge->v0->get<1>() - floor(edge->v0->get<1>()));
        edge->nextRow = floor(edge->v1->get<1>()) + 1;
        for(auto vertexIdx = 0u; vertexIdx < verticesCount; vertexIdx++)
        {
            const auto& v = vertices[vertexIdx];

            if(v.get<1>() > edge->v0->get<1>() && floor(v.get<1>()) < edge->nextRow)
                edge->nextRow = floor(v.get<1>());
        }
        //LogPrintf(LogSeverityLevel::Debug, "Edge %p y(%d %d)(%f %f), next row = %d", edge, edge->startRow, edge->endRow, edge->v0->y, edge->v1->y, edge->nextRow);
        edges.push_back(edge);
    }

    // Sort edges by ascending Y
    std::sort(edges.begin(), edges.end(), [](Edge* l, Edge* r) -> bool
    {
        return l->v0->get<1>() > r->v0->get<1>();
    });

    // Loop in [yMin .. yMax]
    std::vector<Edge*> aet;
    aet.reserve(edges.size());
    for(auto rowIdx = rowMin; rowIdx <= rowMax;)
    {
        //LogPrintf(LogSeverityLevel::Debug, "------------------ %d -----------------", rowIdx);

        // Find active edges
        int nextRow = rowMax;
        for(auto itEdge = edges.cbegin(); itEdge != edges.cend(); ++itEdge)
        {
            auto edge = *itEdge;

            const auto isHorizontal = (edge->startRow == edge->endRow);
            if(nextRow > edge->nextRow && edge->nextRow > rowIdx && !isHorizontal)
                nextRow = edge->nextRow;

            if(edge->startRow != rowIdx)
                continue;

            if(isHorizontal)
            {
                // Fill horizontal edge
                const auto xMin = floor(std::max(edge->v0->get<0>(), edge->v1->get<0>()));
                const auto xMax = floor(std::min(edge->v0->get<0>(), edge->v1->get<0>()));
                /*for(auto x = xMin; x <= xMax; x++)
                    fillPoint(PointI(x, rowIdx));*/
                continue;
            }

            //LogPrintf(LogSeverityLevel::Debug, "line %d. Adding edge %p y(%f %f)", rowIdx, edge, edge->v0->y, edge->v1->y);
            aet.push_back(edge);
            continue;
        }

        // If there are no active edges, we've finished filling
        if(aet.empty())
            break;
        assert(aet.size() % 2 == 0);

        // Sort aet by X
        std::sort(aet.begin(), aet.end(), [](Edge* l, Edge* r) -> bool
        {
            return l->v0->get<0>() > r->v0->get<0>();
        });
        
        // Find next row
        for(; rowIdx < nextRow; rowIdx++)
        {
            const unsigned int pairsCount = aet.size() / 2;

            auto itEdgeL = aet.cbegin();
            auto itEdgeR = itEdgeL + 1;

            for(auto pairIdx = 0u; pairIdx < pairsCount; pairIdx++, itEdgeL = ++itEdgeR, ++itEdgeR)
            {
                auto lEdge = *itEdgeL;
                auto rEdge = *itEdgeR;

                // Fill from l to r
                auto lXf = lEdge->xOrigin + (rowIdx - lEdge->startRow + 0.5f) * lEdge->slope;
                auto rXf = rEdge->xOrigin + (rowIdx - rEdge->startRow + 0.5f) * rEdge->slope;
                auto xMinF = std::min(lXf, rXf);
                auto xMaxF = std::max(lXf, rXf);
                auto xMin = floor(xMinF);
                auto xMax = floor(xMaxF);
                
                //LogPrintf(LogSeverityLevel::Debug, "line %d from %d(%f) to %d(%f)", rowIdx, xMin, xMinF, xMax, xMaxF);
                /*for(auto x = xMin; x <= xMax; x++)
                    fillPoint(PointI(x, rowIdx));*/
            }
        }

        // Deactivate those edges that have end at yNext
        for(auto itEdge = aet.begin(); itEdge != aet.end();)
        {
            auto edge = *itEdge;

            if(edge->endRow <= nextRow)
            {
                // When we're done processing the edge, fill it Y-by-X
                auto startCol = floor(edge->v0->get<0>());
                auto endCol = floor(edge->v1->get<0>());
                auto revSlope = 1.0f / edge->slope;
                auto yOrigin = edge->v0->get<1>() - revSlope * (edge->v0->get<0>() - floor(edge->v0->get<0>()));
                auto xMax = std::max(startCol, endCol);
                auto xMin = std::min(startCol, endCol);
                for(auto colIdx = xMin; colIdx <= xMax; colIdx++)
                {
                    auto yf = yOrigin + (colIdx - startCol + 0.5f) * revSlope;
                    auto y = floor(yf);

                    //LogPrintf(LogSeverityLevel::Debug, "col %d(s%d) added Y = %d (%f)", colIdx, colIdx - startCol, y, yf);
                    fillPoint(pointI(colIdx, y));
                }

                //////////////////////////////////////////////////////////////////////////
                auto yMax = std::max(edge->startRow, edge->endRow);
                auto yMin = std::min(edge->startRow, edge->endRow);
                for(auto rowIdx_ = yMin; rowIdx_ <= yMax; rowIdx_++)
                {
                    auto xf = edge->xOrigin + (rowIdx_ - edge->startRow + 0.5f) * edge->slope;
                    auto x = floor(xf);

                    //LogPrintf(LogSeverityLevel::Debug, "row %d(s%d) added Y = %d (%f)", rowIdx_, rowIdx_ - edge->startRow, x, xf);
                    fillPoint(pointI(x, rowIdx_));
                }
                //////////////////////////////////////////////////////////////////////////

                //LogPrintf(LogSeverityLevel::Debug, "line %d. Removing edge %p y(%f %f)", rowIdx, edge, edge->v0->y, edge->v1->y);
                itEdge = aet.erase(itEdge);
            }
            else
                ++itEdge;
        }
    }

    // Cleanup
    for(auto itEdge = edges.cbegin(); itEdge != edges.cend(); ++itEdge)
        delete *itEdge;
}

#pragma pop_macro("max")
#pragma pop_macro("min")
