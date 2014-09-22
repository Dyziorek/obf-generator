#pragma once
#include <math.h>

#define M_PI       3.14159265358979323846

#include <boost/filesystem.hpp>

namespace Tools
{
        double toRadians(const double angle);
        int32_t get31TileNumberX(const double longitude);
        int32_t get31TileNumberY(const double latitude);
        double get31LongitudeX(const double x);
        double get31LatitudeY(const double y);
        double getTileNumberX(const float zoom, const double longitude);
        double getTileNumberY(const float zoom, double latitude);
        double normalizeLatitude(double latitude);
        double normalizeLongitude(double longitude);
        double getPowZoom(const float zoom);
        double getLongitudeFromTile(const float zoom, const double x);
        double getLatitudeFromTile(const float zoom, const double y);
        double x31toMeters(const int32_t x31);
        double y31toMeters(const int32_t y31);
        double squareDistance31(const int32_t x31a, const int32_t y31a, const int32_t x31b, const int32_t y31b);
        double distance31(const int32_t x31a, const int32_t y31a, const int32_t x31b, const int32_t y31b);
        double squareDistance31(const pointI& a, const pointI& b);
        double distance31(const pointI& a, const pointI& b);
        double distance(const double xLonA, const double yLatA, const double xLonB, const double yLatB);
        double projection31(const int32_t x31a, const int32_t y31a, const int32_t x31b, const int32_t y31b, const int32_t x31c, const int32_t y31c);
        double normalizedAngleRadians(double angle);
        double normalizedAngleDegrees(double angle);
        double polygonArea(const std::vector<pointI>& points);
        bool rayIntersectX(const pointF& v0, const pointF& v1, float mY, float& mX);
        bool rayIntersect(const pointF& v0, const pointF& v1, const pointF& v);
        bool rayIntersectX(const pointI& v0, const pointI& v1, int32_t mY, int32_t& mX);
        bool rayIntersect(const pointI& v0, const pointI& v1, const pointI& v);
        double degreesDiff(const double a1, const double a2);
        AreaI tileBoundingBox31(const TileId tileId, const ZoomLevel zoom);
        AreaI areaRightShift(const AreaI& input, const uint32_t shift);
        AreaI areaLeftShift(const AreaI& input, const uint32_t shift);
        uint32_t getNextPowerOfTwo(const uint32_t value);
        double getMetersPerTileUnit(const float zoom, const double yTile, const double unitsPerTile);
        TileId normalizeTileId(const TileId input, const ZoomLevel zoom);
        pointI normalizeCoordinates(const pointI& input, const ZoomLevel zoom);
        pointI normalizeCoordinates(const pointL& input, const ZoomLevel zoom);
        int qAbsCeil(double v);
        int qAbsFloor(double v);
        enum CHCode : uint8_t
        {
            Inside = 0, // 0000
            Left = 1,   // 0001
            Right = 2,  // 0010
            Bottom = 4, // 0100
            Top = 8,    // 1000
        };
        uint8_t computeCohenSutherlandCode(const pointI& p, const AreaI& box);
        std::set<ZoomLevel> enumerateZoomLevels(const ZoomLevel from, const ZoomLevel to);

        inline double toRadians(const double angle)
        {
            return MapUtils::toRadians(angle);
        }

        inline int32_t get31TileNumberX(const double longitude)
        {
			return MapUtils::get31TileNumberX(longitude);
        }

        inline int32_t get31TileNumberY(const double latitude)
        {
			return MapUtils::get31TileNumberY(latitude);
        }

        inline double get31LongitudeX(const double x)
        {
            return MapUtils::getLongitudeFromTile(21, x / 1024.);
        }

        inline double get31LatitudeY(const double y)
        {
            return MapUtils::getLatitudeFromTile(21, y / 1024.);
        }

        inline double getTileNumberX(const float zoom, const double longitude)
        {
			return MapUtils::getTileNumberX(zoom, longitude);
        }

        inline double getTileNumberY(const float zoom, double latitude)
        {
			return MapUtils::getTileNumberY(zoom, latitude);
        }

        inline double normalizeLatitude(double latitude)
        {
            while(latitude < -90.0 || latitude > 90.0)
            {
                if(latitude < 0.0)
                    latitude += 180.0;
                else
                    latitude -= 180.0;
            }

            if(latitude < -85.0511)
                return -85.0511;
            else if(latitude > 85.0511)
                return 85.0511;

            return latitude;
        }

        inline double normalizeLongitude(double longitude)
        {
            while(longitude < -180.0 || longitude >= 180.0)
            {
                if(longitude < 0.0)
                    longitude += 360.0;
                else
                    longitude -= 360.0;
            }
            return longitude;
        }

        inline double getPowZoom(const float zoom)
        {
			return MapUtils::getPowZoom(zoom);
        }

        inline double getLongitudeFromTile(const float zoom, const double x)
        {
            return x / getPowZoom(zoom) * 360.0 - 180.0;
        }

        inline double getLatitudeFromTile(const float zoom, const double y)
        {
            int sign = y < 0 ? -1 : 1;
            double result = atan(sign * sinh(M_PI * (1 - 2 * y / getPowZoom(zoom)))) * 180. / M_PI;
            return result;
        }

        inline double x31toMeters(const int32_t x31)
        {
            return static_cast<double>(x31)* 0.011;
        }

        inline double y31toMeters(const int32_t y31)
        {
            return static_cast<double>(y31)* 0.01863;
        }

        inline double squareDistance31(const int32_t x31a, const int32_t y31a, const int32_t x31b, const int32_t y31b)
        {
            const auto dx = x31toMeters(x31a - x31b);
            const auto dy = y31toMeters(y31a - y31b);
            return dx * dx + dy * dy;
        }

        inline double distance31(const int32_t x31a, const int32_t y31a, const int32_t x31b, const int32_t y31b)
        {
            return sqrt(squareDistance31(x31a, y31a, x31b, y31b));
        }

        inline double squareDistance31(const pointI& a, const pointI& b)
        {
            const auto dx = x31toMeters(a.get<0>() - b.get<0>());
            const auto dy = y31toMeters(a.get<1>() - b.get<1>());
            return dx * dx + dy * dy;
        }

        inline double distance31(const pointI& a, const pointI& b)
        {
            return sqrt(squareDistance31(a, b));
        }

        inline double distance(const double xLonA, const double yLatA, const double xLonB, const double yLatB)
        {
            double R = 6371; // km
            double dLat = toRadians(yLatB - yLatA);
            double dLon = toRadians(xLonB - xLonA);
            double a =
                sin(dLat / 2.0) * sin(dLat / 2.0) +
                cos(toRadians(yLatA)) * cos(toRadians(yLatB)) *
                sin(dLon / 2.0) * sin(dLon / 2.0);
			double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
            return R * c * 1000.0;
        }

        inline double projection31(const int32_t x31a, const int32_t y31a, const int32_t x31b, const int32_t y31b, const int32_t x31c, const int32_t y31c)
        {
            // Scalar multiplication between (AB, AC)
            auto p =
                x31toMeters(x31b - x31a) * x31toMeters(x31c - x31a) +
                y31toMeters(y31b - y31a) * y31toMeters(y31c - y31a);
            return p;
        }

        inline double normalizedAngleRadians(double angle)
        {
            while(angle > M_PI)
                angle -= 2.0 * M_PI;
            while(angle <= -M_PI)
                angle += 2.0 * M_PI;
            return angle;
        }

        inline double normalizedAngleDegrees(double angle)
        {
            while(angle > 180.0)
                angle -= 360.0;
            while(angle <= -180.0)
                angle += 360.;
            return angle;
        }

        inline double polygonArea(const std::vector<pointI>& points)
        {
            double area = 0.0;

			//assert(points.front() == points.back());

            auto p0 = points.data();
            auto p1 = p0 + 1;
            for(int idx = 1; idx < points.size(); idx++, p0++, p1++)
            {
				area += static_cast<double>(p0->get<0>()) * static_cast<double>(p1->get<1>()) - static_cast<double>(p1->get<0>()) * static_cast<double>(p0->get<1>());
            }
            area = abs(area) * 0.5;

            return area;
        }

        inline bool rayIntersectX(const pointF& v0_, const pointF& v1_, float mY, float& mX)
        {
            // prev node above line
            // x,y node below line

            const auto& v0 = (v0_.get<1>() > v1_.get<1>()) ? v1_ : v0_;
            const auto& v1 = (v0_.get<1>() > v1_.get<1>()) ? v0_ : v1_;

            if(MapUtils::fuzzyCompare(v1.get<1>(), mY) || MapUtils::fuzzyCompare(v0.get<1>(), mY))
                mY -= 1.0f;

            if(v0.get<1>() > mY || v1.get<1>() < mY)
                return false;

            if(boost::geometry::equals(v1,v0))
            {
                // the node on the boundary !!!
                mX = v1.get<0>();
                return true;
            }

            // that tested on all cases (left/right)
            mX = v1.get<0>() + (mY - v1.get<1>()) * (v1.get<0>() - v0.get<0>()) / (v1.get<1>() - v0.get<1>());
            return true;
        }

        inline bool rayIntersect(const pointF& v0, const pointF& v1, const pointF& v)
        {
            float t;
            if(!rayIntersectX(v0, v1, v.get<1>(), t))
                return false;

            if(t < v.get<0>())
                return true;

            return false;
        }

        inline bool rayIntersectX(const pointI& v0_, const pointI& v1_, int32_t mY, int32_t& mX)
        {
            // prev node above line
            // x,y node below line

            const auto& v0 = (v0_.get<1>() > v1_.get<1>()) ? v1_ : v0_;
            const auto& v1 = (v0_.get<1>() > v1_.get<1>()) ? v0_ : v1_;

            if(v1.get<1>() == mY || v0.get<1>() == mY)
                mY -= 1;

            if(v0.get<1>() > mY || v1.get<1>() < mY)
                return false;

            if(boost::geometry::equals(v1,v0))
            {
                // the node on the boundary !!!
                mX = v1.get<0>();
                return true;
            }

            // that tested on all cases (left/right)
            mX = static_cast<int32_t>(v1.get<0>() + static_cast<double>(mY - v1.get<1>()) * static_cast<double>(v1.get<0>() - v0.get<0>()) / static_cast<double>(v1.get<1>() - v0.get<1>()));
            return true;
        }

        inline bool rayIntersect(const pointI& v0, const pointI& v1, const pointI& v)
        {
            int32_t t;
            if(!rayIntersectX(v0, v1, v.get<1>(), t))
                return false;

            if(t < v.get<0>())
                return true;

            return false;
        }

        inline double degreesDiff(const double a1, const double a2)
        {
            auto diff = a1 - a2;
            while(diff > 180.0)
                diff -= 360.0;
            while(diff <= -180.0)
                diff += 360.0;
            return diff;
        }

        inline AreaI tileBoundingBox31(const TileId tileId, const ZoomLevel zoom)
        {
            AreaI output;

            const auto zoomShift = ZoomLevel31 - zoom;

            output.min_corner().set<1>(tileId.y << zoomShift);
            output.min_corner().set<0>(tileId.x << zoomShift);
            output.max_corner().set<1>(((tileId.y + 1) << zoomShift) - 1);
            output.max_corner().set<0>(((tileId.x + 1) << zoomShift) - 1);

            //assert(output.top >= 0 && output.top <= std::numeric_limits<int32_t>::max());
            //assert(output.left >= 0 && output.left <= std::numeric_limits<int32_t>::max());
            //assert(output.bottom >= 0 && output.bottom <= std::numeric_limits<int32_t>::max());
            //assert(output.right >= 0 && output.right <= std::numeric_limits<int32_t>::max());
            //assert(output.right >= output.left);
            //assert(output.bottom >= output.top);

            return output;
        }

        inline AreaI areaRightShift(const AreaI& input, const uint32_t shift)
        {
            AreaI output;
            uint32_t tail;

            output.min_corner().set<1>(input.min_corner().get<1>() >> shift);
            output.min_corner().set<0>(input.min_corner().get<0>() >> shift);

            tail = input.max_corner().get<1>() & ((1 << shift) - 1);
            output.max_corner().set<1>((input.max_corner().get<1>() >> shift) + (tail ? 1 : 0));
            tail = input.max_corner().get<0>() & ((1 << shift) - 1);
            output.max_corner().set<0>((input.max_corner().get<0>() >> shift) + (tail ? 1 : 0));

            //assert(output.top >= 0 && output.top <= std::numeric_limits<int32_t>::max());
            //assert(output.left >= 0 && output.left <= std::numeric_limits<int32_t>::max());
            //assert(output.bottom >= 0 && output.bottom <= std::numeric_limits<int32_t>::max());
            //assert(output.right >= 0 && output.right <= std::numeric_limits<int32_t>::max());
            //assert(output.right >= output.left);
            //assert(output.bottom >= output.top);

            return output;
        }

        inline AreaI areaLeftShift(const AreaI& input, const uint32_t shift)
        {
            AreaI output;

            output.min_corner().set<1>(input.min_corner().get<1>() << shift);
            output.min_corner().set<0>(input.min_corner().get<0>() << shift);
            output.max_corner().set<1>(input.max_corner().get<1>() << shift);
            output.max_corner().set<0>(input.max_corner().get<1>() << shift);

            //assert(output.top >= 0 && output.top <= std::numeric_limits<int32_t>::max());
            //assert(output.left >= 0 && output.left <= std::numeric_limits<int32_t>::max());
            //assert(output.bottom >= 0 && output.bottom <= std::numeric_limits<int32_t>::max());
            //assert(output.right >= 0 && output.right <= std::numeric_limits<int32_t>::max());
            //assert(output.right >= output.left);
            //assert(output.bottom >= output.top);

            return output;
        }

        inline uint32_t getNextPowerOfTwo(const uint32_t value)
        {
            if(value == 0)
                return 0;

            auto n = value;

            n--;
            n |= n >> 1;
            n |= n >> 2;
            n |= n >> 4;
            n |= n >> 8;
            n |= n >> 16;
            n++;

            return n;
        }

        inline double getMetersPerTileUnit(const float zoom, const double yTile, const double unitsPerTile)
        {
            // Equatorial circumference of the Earth in meters
            const static double C = 40075017.0;

            const auto powZoom = getPowZoom(zoom);
            const auto sinhValue = sinh((2.0 * M_PI * yTile) / powZoom - M_PI);
            auto res = C / (powZoom * unitsPerTile * sqrt(sinhValue*sinhValue + 1.0));

            return res;
        }

        inline TileId normalizeTileId(const TileId input, const ZoomLevel zoom)
        {
            TileId output = input;

            const auto tilesCount = static_cast<int32_t>(1u << zoom);

            while(output.x < 0)
				output.x += tilesCount;
			while(output.y < 0)
                output.y += tilesCount;

            // Max zoom level (31) is skipped, since value stored in int31 can not be more than tilesCount(31)
            if(zoom < ZoomLevel31)
            {
                while(output.x >= tilesCount)
                    output.x -= tilesCount;
                while(output.y >= tilesCount)
                    output.y -= tilesCount;
            }

            //assert(output.get<0>() >= 0 && ((zoom < ZoomLevel31 && output.get<0>() < tilesCount) || (zoom == ZoomLevel31)));
            //assert(output.get<0>() >= 0 && ((zoom < ZoomLevel31 && output.get<1>() < tilesCount) || (zoom == ZoomLevel31)));

            return output;
        }

        inline pointI normalizeCoordinates(const pointI& input, const ZoomLevel zoom)
        {
            pointI output = input;

            const auto tilesCount = static_cast<int32_t>(1u << zoom);

            while(output.get<0>() < 0)
                output.set<0>(output.get<0>() + tilesCount);
            while(output.get<1>() < 0)
                output.set<1>(output.get<1>() + tilesCount);

            // Max zoom level (31) is skipped, since value stored in int31 can not be more than tilesCount(31)
            if(zoom < ZoomLevel31)
            {
                while(output.get<0>() >= tilesCount)
                    output.set<0>(output.get<0>() - tilesCount);
                while(output.get<1>() >= tilesCount)
                    output.set<1>(output.get<1>() - tilesCount);
            }

            assert(output.get<0>() >= 0 && ((zoom < ZoomLevel31 && output.get<0>() < tilesCount) || (zoom == ZoomLevel31)));
            assert(output.get<0>() >= 0 && ((zoom < ZoomLevel31 && output.get<1>() < tilesCount) || (zoom == ZoomLevel31)));

            return output;
        }

        inline pointI normalizeCoordinates(const pointL& input, const ZoomLevel zoom)
        {
            pointL output = input;

            const auto tilesCount = static_cast<int64_t>(1ull << zoom);

            while(output.get<0>() < 0)
                output.set<0>(output.get<0>() + tilesCount);
            while(output.get<1>() < 0)
                output.set<1>(output.get<1>() + tilesCount);

            while(output.get<0>() >= tilesCount)
                output.set<0>(output.get<0>() - tilesCount);
            while(output.get<1>() >= tilesCount)
                output.set<1>(output.get<1>() - tilesCount);

            assert(output.get<0>() >= 0 && output.get<0>() < tilesCount);
            assert(output.get<1>() >= 0 && output.get<1>() < tilesCount);

            return pointI(static_cast<int32_t>(output.get<0>()), static_cast<int32_t>(output.get<1>()));
        }

        inline int qAbsCeil(double v)
        {
            return v > 0 ? std::ceil(v) : std::floor(v);
        }
        inline int qAbsFloor(double v)
        {
            return v > 0 ? std::floor(v) : std::ceil(v);
        }

        inline uint8_t computeCohenSutherlandCode(const pointI& p, const AreaI& box)
        {
            uint8_t res = CHCode::Inside;

            if(p.get<0>() < box.min_corner().get<0>())           // to the left of clip box
                res |= CHCode::Left;
            else if(p.get<0>() > box.max_corner().get<0>())     // to the right of clip box
                res |= CHCode::Right;
            if(p.get<1>() < box.min_corner().get<1>())            // above the clip box
                res |= CHCode::Bottom;
            else if(p.get<1>() > box.max_corner().get<1>())    // below the clip box
                res |= CHCode::Top;

            return res;
        }

		 bool  extractFirstNumberPosition(const std::string& value, int& first, int& last, bool allowSigned, bool allowDot);
         double  parseSpeed(const std::string& value, const double defValue, bool* wasParsed = nullptr);
         double  parseLength(const std::string& value, const double defValue, bool* wasParsed = nullptr);
         double  parseWeight(const std::string& value, const double defValue, bool* wasParsed = nullptr);
         int  parseArbitraryInt(const std::string& value, const int defValue, bool* wasParsed = nullptr);
         long  parseArbitraryLong(const std::string& value, const long defValue, bool* wasParsed = nullptr);
         unsigned int  parseArbitraryUInt(const std::string& value, const unsigned int defValue, bool* wasParsed = nullptr);
         unsigned long  parseArbitraryULong(const std::string& value, const unsigned long defValue, bool* wasParsed = nullptr);
         float  parseArbitraryFloat(const std::string& value, const float defValue, bool* wasParsed = nullptr);
         bool  parseArbitraryBool(const std::string& value, const bool defValue, bool* wasParsed = nullptr);

         int  javaDoubleCompare(const double l, const double r);
         void  findFiles(const boost::filesystem::path& origin, const std::vector<std::string>& masks, std::vector<std::string>& files, const bool recursively = true);

         void  scanlineFillPolygon(const unsigned int verticesCount, const pointF* const vertices, std::function<void(const pointI&)> fillPoint);

        inline std::set<ZoomLevel> enumerateZoomLevels(const ZoomLevel from, const ZoomLevel to)
        {
            std::set<ZoomLevel> result;
            for(int level = from; level <= to; level++)
                result.insert(static_cast<ZoomLevel>(level));

            return result;
        }


		class uint32_from_hex   // For use with boost::lexical_cast
		{
			uint32_t value;
		public:
			operator uint32_t() const { return value; }
			friend std::istream& operator>>( std::istream& in, uint32_from_hex& outValue )
			{
				in >> std::hex >> outValue.value;
				return in;
			}
		};

		class int32_from_hex   // For use with boost::lexical_cast
		{
			uint32_t value;
		public:
			operator uint32_t() const { return static_cast<int32_t>( value ); }
			friend std::istream& operator>>( std::istream& in, int32_from_hex& outValue )
			{
				in >> std::hex >> outValue.value;
			}
		};

    } // namespace Utilities
