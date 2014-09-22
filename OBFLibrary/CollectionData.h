/**
 * @file
 *
 * @section LICENSE
 *
 * OsmAnd - Android navigation software based on OSM maps.
 * Copyright (C) 2010-2014  OsmAnd Authors listed in AUTHORS file
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include <cassert>
#include <array>
#include <functional>
#include <type_traits>
#include <unordered_map>
#include <boost\thread.hpp>
#include <mutex>
#include <atomic>
union TileId
{
	uint64_t id;
	struct
	{
		uint32_t x;
		uint32_t y;
	};
};

 enum ZoomLevel:  int32_t
    {
        ZoomLevel0 = 0,
        ZoomLevel1,
        ZoomLevel2,
        ZoomLevel3,
        ZoomLevel4,
        ZoomLevel5,
        ZoomLevel6,
        ZoomLevel7,
        ZoomLevel8,
        ZoomLevel9,
        ZoomLevel10,
        ZoomLevel11,
        ZoomLevel12,
        ZoomLevel13,
        ZoomLevel14,
        ZoomLevel15,
        ZoomLevel16,
        ZoomLevel17,
        ZoomLevel18,
        ZoomLevel19,
        ZoomLevel20,
        ZoomLevel21,
        ZoomLevel22,
        ZoomLevel23,
        ZoomLevel24,
        ZoomLevel25,
        ZoomLevel26,
        ZoomLevel27,
        ZoomLevel28,
        ZoomLevel29,
        ZoomLevel30,
        ZoomLevel31 = 31,

        InvalidZoom = -1,
        MinZoomLevel = ZoomLevel0,
        MaxZoomLevel = ZoomLevel31,
    };
    enum {
        ZoomLevelsCount = static_cast<unsigned>(ZoomLevel::MaxZoomLevel) + 1u
    };

    template<class ENTRY>
    class TilesCollectionEntry;

    template<class ENTRY>
    class TilesCollection
    {
    public:
        class Link : public std::enable_shared_from_this< Link >
        {
        private:
        protected:
        public:
            Link(TilesCollection< ENTRY >& collection_)
                : collection(collection_)
            {}

            virtual ~Link()
            {}

            TilesCollection< ENTRY >& collection;
        };

    private:
        std::array< std::unordered_map< TileId, std::shared_ptr<ENTRY> >, ZoomLevelsCount > _zoomLevels;
		mutable boost::shared_mutex _collectionLock;
    protected:
        const std::shared_ptr< Link > _link;
    public:
        TilesCollection()
            : _link(new Link(*this))
        {}
        virtual ~TilesCollection()
        {}

        virtual bool obtainEntry(std::shared_ptr<ENTRY>& outEntry, const TileId tileId, const ZoomLevel zoom) const
        {
			boost::shared_lock<boost::shared_mutex> scopedLocker(&_collectionLock);

            const auto& zoomLevel = _zoomLevels[zoom];
            const auto itEntry = zoomLevel.constFind(tileId);
            if(itEntry != zoomLevel.cend())
            {
                outEntry = *itEntry;

                return true;
            }

            return false;
        }

        virtual void obtainOrAllocateEntry(std::shared_ptr<ENTRY>& outEntry, const TileId tileId, const ZoomLevel zoom, std::function<ENTRY* (const TilesCollection<ENTRY>&, const TileId, const ZoomLevel)> allocator)
        {
            assert(allocator != nullptr);

            boost::shared_lock<boost::shared_mutex>  scopedLocker(&_collectionLock);

            auto& zoomLevel = _zoomLevels[zoom];
            auto itEntry = zoomLevel.constFind(tileId);
            if(itEntry != zoomLevel.cend())
            {
                outEntry = *itEntry;
                return;
            }

            auto newEntry = allocator(*this, tileId, zoom);
            outEntry.reset(newEntry);
            itEntry = zoomLevel.insert(tileId, outEntry);
        }

        virtual void obtainEntries(std::list< std::shared_ptr<ENTRY> >* outList, std::function<bool (const std::shared_ptr<ENTRY>& entry, bool& cancel)> filter = nullptr) const
        {
            boost::shared_lock<boost::shared_mutex> scopedLocker(&_collectionLock);

            bool doCancel = false;
            for(auto itZoomLevel = _zoomLevels.cbegin(); itZoomLevel != _zoomLevels.cend(); ++itZoomLevel)
            {
                const auto& zoomLevel = *itZoomLevel;

                for(auto itEntryPair = zoomLevel.cbegin(); itEntryPair != zoomLevel.cend(); ++itEntryPair)
                {
                    const auto& entry = itEntryPair.value();
                    
                    if(!filter || (filter && filter(entry, doCancel)))
                    {
                        if(outList)
                            outList->push_back(entry);
                    }

                    if(doCancel)
                        return;
                }
            }
        }

        virtual void removeAllEntries()
        {
			boost::upgrade_lock<boost::shared_mutex> scopedLocker(&_collectionLock);
			
			boost::upgrade_to_unique_lock<boost::shared_mutex> upgradeLock(scopedLocker);
            for(int zoom = ZoomLevel::MinZoomLevel; zoom != ZoomLevel::MaxZoomLevel; zoom++)
            {
                auto& zoomLevel = _zoomLevels[zoom];

                for(auto itEntry = zoomLevel.cbegin(); itEntry != zoomLevel.cend(); ++itEntry)
                    itEntry.value()->unlink();

                zoomLevel.clear();
            }
        }

        virtual void removeEntry(const TileId tileId, const ZoomLevel zoom)
        {
           boost::upgrade_lock<boost::shared_mutex> scopedLocker(&_collectionLock);
			
			boost::upgrade_to_unique_lock<boost::shared_mutex> upgradeLock(scopedLocker);

            auto& zoomLevel = _zoomLevels[zoom];
            const auto itEntry = zoomLevel.find(tileId);
            if(itEntry == zoomLevel.end())
                return;

            itEntry.value()->unlink();
            zoomLevel.erase(itEntry);
        }

        virtual void removeEntries(std::function<bool (const std::shared_ptr<ENTRY>& entry, bool& cancel)> filter = nullptr)
        {
            boost::upgrade_lock<boost::shared_mutex> scopedLocker(&_collectionLock);
			
			boost::upgrade_to_unique_lock<boost::shared_mutex> upgradeLock(scopedLocker);

            bool doCancel = false;
            for(auto itZoomLevel = _zoomLevels.begin(); itZoomLevel != _zoomLevels.end(); ++itZoomLevel)
            {
                auto& zoomLevel = *itZoomLevel;

                QMutableHashIterator< TileId, std::shared_ptr<ENTRY> > itEntryPair(zoomLevel);
                while(itEntryPair.hasNext())
                {
                    itEntryPair.next();

                    const auto doRemove = (filter == nullptr) || filter(itEntryPair.value(), doCancel);
                    if(doRemove)
                    {
                        itEntryPair.value()->unlink();
                        itEntryPair.remove();
                    }

                    if(doCancel)
                        return;
                }
            }
        }

    friend class TilesCollectionEntry<ENTRY>;
    };

    template<class ENTRY>
    class TilesCollectionEntry
    {
    public:
        typedef typename TilesCollection<ENTRY>::Link Link;

    private:
        std::weak_ptr<Link> _link;
        
        void unlink()
        {
            detach();

            assert(checkIsSafeToUnlink());
            _link.reset();
        }
    protected:
        TilesCollectionEntry(const TilesCollection<ENTRY>& collection, const TileId tileId_, const ZoomLevel zoom_)
            : _link(collection._link)
            , link(_link)
            , tileId(tileId_)
            , zoom(zoom_)
        {
        }

        virtual void detach()
        {
        }

        virtual bool checkIsSafeToUnlink()
        {
            return true;
        }

        void safeUnlink()
        {
            if(_link.expired())
                return;

            unlink();
            return;
        }
    public:
        virtual ~TilesCollectionEntry()
        {
            assert(_link.expired());
        }

        const std::weak_ptr<Link>& link;

        const TileId tileId;
        const ZoomLevel zoom;

    friend class TilesCollection<ENTRY>;
    };

    template<class ENTRY, typename STATE_ENUM, STATE_ENUM UNDEFINED_STATE_VALUE>
    class TilesCollectionEntryWithState : public TilesCollectionEntry<ENTRY>
    {
    private:
    protected:
		std::atomic_int8_t _stateValue;
    public:
        TilesCollectionEntryWithState(const TilesCollection<ENTRY>& collection, const TileId tileId, const ZoomLevel zoom, const STATE_ENUM state = UNDEFINED_STATE_VALUE)
            : TilesCollectionEntry<ENTRY>(collection, tileId, zoom)
            , _stateValue(static_cast<int>(state))
        {}
        virtual ~TilesCollectionEntryWithState()
        {}

        inline STATE_ENUM getState() const
        {
            return static_cast<STATE_ENUM>(_stateValue.load());
        }

        inline void setState(const STATE_ENUM newState)
        {
            _stateValue.store(static_cast<int>(newState));
        }

        inline bool setStateIf(const STATE_ENUM testState, const STATE_ENUM newState)
        {
			return _stateValue.compare_exchange_strong(testState, newState, boost::memory_order::memory_order_acq_rel);
        }
    };

