/*  XMMS2 - X Music Multiplexer System
 *  Copyright (C) 2003-2006 XMMS2 Team
 *
 *  PLUGINS ARE NOT CONSIDERED TO BE DERIVED WORK !!!
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 */

#ifndef XMMSCLIENTPP_COLLECTION_H
#define XMMSCLIENTPP_COLLECTION_H

#include <xmmsclient/xmmsclient.h>
#include <xmmsclient/xmmsclient++/signal.h>
#include <xmmsclient/xmmsclient++/typedefs.h>
#include <xmmsclient/xmmsclient++/dict.h>
#include <xmmsclient/xmmsclient++/mainloop.h>
#include <xmmsclient/xmmsclient++/exceptions.h>

#include <list>
#include <string>

#include <boost/shared_ptr.hpp>

namespace Xmms 
{

	class Client;

	/** @class Collection collection.h "xmmsclient/xmmsclient++/collection.h"
	 *  @brief This class controls the collections on the server.
	 */
	class Collection
	{

		public:

			static CollPtr createColl( xmmsc_result_t* res );
			static CollPtr createColl( xmmsc_coll_t* coll );

			typedef xmmsc_coll_namespace_t Namespace;

			static const Namespace ALL;
			static const Namespace COLLECTIONS;
			static const Namespace PLAYLISTS;


			/** Destructor.
			 */
			virtual ~Collection();

			/** Get the structure of a collection saved on the server.
			 *
			 *  @param name The name of the collection on the server.
			 *  @param nsname The namespace in which to look for the collection.
			 *
			 *  @throw connection_error If the client isn't connected.
			 *  @throw mainloop_running_error If a mainloop is running -
			 *  sync functions can't be called when mainloop is running. This
			 *  is only thrown if the programmer is careless or doesn't know
			 *  what he/she's doing. (logic_error)
			 *  @throw result_error If the operation failed.
			 *
			 *  @return a pointer to a Coll::Coll object representing
			 *  the collection structure.
			 */
			CollPtr
			get( const std::string& name, Namespace nsname ) const;

			/** List the names of collections saved in the given namespace.
			 *
			 *  @param nsname Namespace to list collection names from.
			 *
			 *  @throw connection_error If the client isn't connected.
			 *  @throw mainloop_running_error If a mainloop is running -
			 *  sync functions can't be called when mainloop is running. This
			 *  is only thrown if the programmer is careless or doesn't know
			 *  what he/she's doing. (logic_error)
			 *  @throw result_error If the operation failed.
			 *
			 *  @return a list of collection names in the given namespace.
			 */
			List< std::string >
			list( Namespace nsname ) const;

			/** Save a collection structure under a given name in a
			 *  certain namespace on the server.
			 *
			 *  @param coll The collection structure to save.
			 *  @param name The name under which to save the collection.
			 *  @param nsname The namespace in which to save the collection.
			 *
			 *  @throw connection_error If the client isn't connected.
			 *  @throw mainloop_running_error If a mainloop is running -
			 *  sync functions can't be called when mainloop is running. This
			 *  is only thrown if the programmer is careless or doesn't know
			 *  what he/she's doing. (logic_error)
			 *  @throw result_error If the operation failed.
			 */
			void
			save( const Coll::Coll& coll,
			      const std::string& name,
			      Namespace nsname ) const;

			/** Remove the collection given its name and its namespace.
			 *
			 *  @param name The name of the collection to remove.
			 *  @param nsname The namespace from which to remove the collection.
			 *
			 *  @throw connection_error If the client isn't connected.
			 *  @throw mainloop_running_error If a mainloop is running -
			 *  sync functions can't be called when mainloop is running. This
			 *  is only thrown if the programmer is careless or doesn't know
			 *  what he/she's doing. (logic_error)
			 *  @throw result_error If the operation failed.
			 */
			void
			remove( const std::string& name, Namespace nsname ) const;

			/** List the names of collections that contain a given media.
			 *
			 *  @param id The id of the media.
			 *  @param nsname The namespace in which to look for collections.
			 *
			 *  @throw connection_error If the client isn't connected.
			 *  @throw mainloop_running_error If a mainloop is running -
			 *  sync functions can't be called when mainloop is running. This
			 *  is only thrown if the programmer is careless or doesn't know
			 *  what he/she's doing. (logic_error)
			 *  @throw result_error If the operation failed.
			 *
			 *  @return a list of collection names.
			 */
			List< std::string >
			find( unsigned int id, Namespace nsname ) const;

			/** Rename the collection given its name and its namespace.
			 *  @note A collection cannot be moved to another namespace.
			 *
			 *  @param from_name The name of the collection to rename.
			 *  @param to_name The new name for the collection.
			 *  @param nsname The namespace in which the collection to rename is.
			 *
			 *  @throw connection_error If the client isn't connected.
			 *  @throw mainloop_running_error If a mainloop is running -
			 *  sync functions can't be called when mainloop is running. This
			 *  is only thrown if the programmer is careless or doesn't know
			 *  what he/she's doing. (logic_error)
			 *  @throw result_error If the operation failed.
			 */
			void
			rename( const std::string& from_name,
			        const std::string& to_name,
			        Namespace nsname ) const;

			/** Import the content of a playlist file (.m3u, .pls,
			 *  etc) into the medialib and return an idlist collection
			 *  containing the imported media.
			 *
			 *  @param path The path to the playlist file.
			 *
			 *  @throw connection_error If the client isn't connected.
			 *  @throw mainloop_running_error If a mainloop is running -
			 *  sync functions can't be called when mainloop is running. This
			 *  is only thrown if the programmer is careless or doesn't know
			 *  what he/she's doing. (logic_error)
			 *  @throw result_error If the operation failed.
			 *
			 *  @return a pointer to a #Coll::Idlist object containing
			 *  the ids of media imported from the playlist file.
			 */
			CollPtr
			idlistFromPlaylistFile( const std::string& path ) const;

			/** Retrieve the ids of media matched by a collection.
			 *  To query the content of a saved collection, use a
			 *  Reference operator.
			 *
			 *  @param coll The collection to query.
			 *  @param order The list of properties by which to order
			 *               the matching media.
			 *  @param limit_len Optional argument to limit the
			 *                   maximum number of media to retrieve.
			 *  @param limit_start Optional argument to set the offset
			 *                     at which to start retrieving media.
			 *
			 *  @throw connection_error If the client isn't connected.
			 *  @throw mainloop_running_error If a mainloop is running -
			 *  sync functions can't be called when mainloop is running. This
			 *  is only thrown if the programmer is careless or doesn't know
			 *  what he/she's doing. (logic_error)
			 *  @throw result_error If the operation failed.
			 *
			 *  @return a list of media ids matched by the collection.
			 */
			List< unsigned int >
			queryIds( const Coll::Coll& coll,
			          const std::list<std::string>& order = std::list<std::string>(),
			          unsigned int limit_len = 0,
			          unsigned int limit_start = 0) const;

			/** Retrieve the properties of media matched by a collection.
			 *  To query the content of a saved collection, use a
			 *  Reference operator.
			 *
			 *  @param coll The collection to query.
			 *  @param order The list of properties by which to order
			 *               the matching media.
			 *  @param limit_len Optional argument to limit the
			 *                   maximum number of media to retrieve.
			 *  @param limit_start Optional argument to set the offset
			 *                     at which to start retrieving media.
			 *  @param fetch The list of properties to retrieve.
			 *  @param group The list of properties by which to group the results.
			 *
			 *  @throw connection_error If the client isn't connected.
			 *  @throw mainloop_running_error If a mainloop is running -
			 *  sync functions can't be called when mainloop is running. This
			 *  is only thrown if the programmer is careless or doesn't know
			 *  what he/she's doing. (logic_error)
			 *  @throw result_error If the operation failed.
			 *
			 *  @return a list of property dicts for each media
			 *  matched by the collection.
			 */
			List< Dict >
			queryInfos( const Coll::Coll& coll,
			            const std::list<std::string>& order = std::list<std::string>(),
			            unsigned int limit_len = 0,
			            unsigned int limit_start = 0,
			            const std::list<std::string>& fetch = std::list<std::string>(),
			            const std::list<std::string>& group = std::list<std::string>()
			          ) const;

			/**
			 * FIXME: Comments
			 */
			CollPtr
			parse( const std::string& pattern ) const;


			/** Get the structure of a collection saved on the server.
			 *
			 *  @param name The name of the collection on the server.
			 *  @param nsname The namespace in which to look for the collection.
			 *  @param slot Function pointer to a function taking a
			 *              const CollPtr& and returning a bool.
			 *  @param error Function pointer to an error callback
			 *               function. (<b>optional</b>)
			 *
			 *  @throw connection_error If the client isn't connected.
			 */
			void
			get( const std::string& name, Namespace nsname,
			     const CollSlot& slot,
			     const ErrorSlot& error = &Xmms::dummy_error
			   ) const;

			/** List the names of collections saved in the given namespace.
			 *
			 *  @param nsname Namespace to list collection names from.
			 *  @param slot Function pointer to a function taking a
			 *              const List<string>& and returning a bool.
			 *  @param error Function pointer to an error callback
			 *               function. (<b>optional</b>)
			 *
			 *  @throw connection_error If the client isn't connected.
			 */
			void
			list( Namespace nsname, const StringListSlot& slot,
			      const ErrorSlot& error = &Xmms::dummy_error ) const;

			/** Save a collection structure under a given name in a
			 *  certain namespace on the server.
			 *
			 *  @param coll The collection structure to save.
			 *  @param name The name under which to save the collection.
			 *  @param nsname The namespace in which to save the collection.
			 *  @param slot Function pointer to a function returning a bool.
			 *  @param error Function pointer to an error callback
			 *               function. (<b>optional</b>)
			 *
			 *  @throw connection_error If the client isn't connected.
			 */
			void
			save( const Coll::Coll& coll, const std::string& name,
			      Namespace nsname, const VoidSlot& slot,
			      const ErrorSlot& error = &Xmms::dummy_error ) const;

			/** Remove the collection given its name and its namespace.
			 *
			 *  @param name The name of the collection to remove.
			 *  @param nsname The namespace from which to remove the collection.
			 *  @param slot Function pointer to a function returning a bool.
			 *  @param error Function pointer to an error callback
			 *               function. (<b>optional</b>)
			 *
			 *  @throw connection_error If the client isn't connected.
			 */
			void
			remove( const std::string& name, Namespace nsname,
			        const VoidSlot& slot,
			        const ErrorSlot& error = &Xmms::dummy_error ) const;

			/** List the names of collections that contain a given media.
			 *
			 *  @param id The id of the media.
			 *  @param nsname The namespace in which to look for collections.
			 *  @param slot Function pointer to a function taking a
			 *              const List<string>& and returning a bool.
			 *  @param error Function pointer to an error callback
			 *               function. (<b>optional</b>)
			 *
			 *  @throw connection_error If the client isn't connected.
			 */
			void
			find( unsigned int id, Namespace nsname,
			      const StringListSlot& slot,
			      const ErrorSlot& error = &Xmms::dummy_error ) const;

			/** Rename the collection given its name and its namespace.
			 *  @note A collection cannot be moved to another namespace.
			 *
			 *  @param from_name The name of the collection to rename.
			 *  @param to_name The new name for the collection.
			 *  @param nsname The namespace in which the collection to rename is.
			 *  @param slot Function pointer to a function returning a bool.
			 *  @param error Function pointer to an error callback
			 *               function. (<b>optional</b>)
			 *
			 *  @throw connection_error If the client isn't connected.
			 */
			void
			rename( const std::string& from_name,
			        const std::string& to_name, Namespace nsname,
			        const VoidSlot& slot,
			        const ErrorSlot& error = &Xmms::dummy_error ) const;

			/** Import the content of a playlist file (.m3u, .pls,
			 *  etc) into the medialib and return an idlist collection
			 *  containing the imported media.
			 *
			 *  @param path The path to the playlist file.
			 *  @param slot Function pointer to a function taking a
			 *              const CollPtr& and returning a bool.
			 *  @param error Function pointer to an error callback
			 *               function. (<b>optional</b>)
			 *
			 *  @throw connection_error If the client isn't connected.
			 */
			void
			idlistFromPlaylistFile( const std::string& path,
			                        const CollSlot& slot,
			                        const ErrorSlot& error = &Xmms::dummy_error
			                      ) const;

			/** Retrieve the ids of media matched by a collection.
			 *  To query the content of a saved collection, use a
			 *  Reference operator.
			 *
			 *  @param coll The collection to query.
			 *  @param order The list of properties by which to order
			 *               the matching media.
			 *  @param limit_len Optional argument to limit the
			 *                   maximum number of media to retrieve.
			 *  @param limit_start Optional argument to set the offset
			 *                     at which to start retrieving media.
			 *  @param slot Function pointer to a function taking a
			 *              const List<unsigned int>& and returning a bool.
			 *  @param error Function pointer to an error callback
			 *               function. (<b>optional</b>)
			 *
			 *  @throw connection_error If the client isn't connected.
			 */
			void
			queryIds( const Coll::Coll& coll,
			          const std::list< std::string >& order,
			          unsigned int limit_len,
			          unsigned int limit_start,
			          const UintListSlot& slot,
			          const ErrorSlot& error = &Xmms::dummy_error ) const;

			/**
			 * @overload
			 * @note No limit_start offset.
			 */
			void
			queryIds( const Coll::Coll& coll,
			          const std::list< std::string >& order,
			          unsigned int limit_len,
			          const UintListSlot& slot,
			          const ErrorSlot& error = &Xmms::dummy_error ) const;

			/**
			 * @overload
			 * @note No limit_start offset or limit_len max number of results.
			 */
			void
			queryIds( const Coll::Coll& coll,
			          const std::list< std::string >& order,
			          const UintListSlot& slot,
			          const ErrorSlot& error = &Xmms::dummy_error ) const;

			/**
			 * @overload
			 * @note No limit_start offset or limit_len max number of
			 *       results, default ordering
			 */
			void
			queryIds( const Coll::Coll& coll,
			          const UintListSlot& slot,
			          const ErrorSlot& error = &Xmms::dummy_error ) const;

			/** Retrieve the properties of media matched by a collection.
			 *  To query the content of a saved collection, use a
			 *  Reference operator.
			 *
			 *  @param coll The collection to query.
			 *  @param order The list of properties by which to order
			 *               the matching media.
			 *  @param limit_len Optional argument to limit the
			 *                   maximum number of media to retrieve.
			 *  @param limit_start Optional argument to set the offset
			 *                     at which to start retrieving media.
			 *  @param fetch The list of properties to retrieve.
			 *  @param group The list of properties by which to group the results.
			 *  @param slot Function pointer to a function taking a
			 *              const DictList& and returning a bool.
			 *  @param error Function pointer to an error callback
			 *               function. (<b>optional</b>)
			 *
			 *  @throw connection_error If the client isn't connected.
			 */
			void
			queryInfos( const Coll::Coll& coll,
			            const std::list< std::string >& order,
			            const std::list< std::string >& fetch,
			            unsigned int limit_len,
			            unsigned int limit_start,
			            const std::list< std::string >& group,
						const DictListSlot& slot,
			            const ErrorSlot& error = &Xmms::dummy_error ) const;

			/**
			 * @overload
			 * @note Without grouping.
			 */
			void
			queryInfos( const Coll::Coll& coll,
			            const std::list< std::string >& order,
			            const std::list< std::string >& fetch,
			            unsigned int limit_len,
			            unsigned int limit_start,
						const DictListSlot& slot,
			            const ErrorSlot& error = &Xmms::dummy_error ) const;

			/**
			 * @overload
			 * @note Without limit.
			 */
			void
			queryInfos( const Coll::Coll& coll,
			            const std::list< std::string >& order,
			            const std::list< std::string >& fetch,
			            const std::list< std::string >& group,
						const DictListSlot& slot,
			            const ErrorSlot& error = &Xmms::dummy_error ) const;

			/**
			 * @overload
			 * @note Without grouping or limit.
			 */
			void
			queryInfos( const Coll::Coll& coll,
			            const std::list< std::string >& order,
			            const std::list< std::string >& fetch,
						const DictListSlot& slot,
			            const ErrorSlot& error = &Xmms::dummy_error ) const;

			/**
			 * @overload
			 * @note Without grouping, limit or list of fetched properties.
			 */
			void
			queryInfos( const Coll::Coll& coll,
			            const std::list< std::string >& order,
						const DictListSlot& slot,
			            const ErrorSlot& error = &Xmms::dummy_error ) const;

			/**
			 * @overload
			 * @note Without grouping, ordering, limit or list of fetched properties.
			 */
			void
			queryInfos( const Coll::Coll& coll,
						const DictListSlot& slot,
			            const ErrorSlot& error = &Xmms::dummy_error ) const;


			/** Request the collection changed broadcast.
			 *
			 *  This will be called if a saved collection structure
			 *  changes on the serverside (saved, updated, renamed,
			 *  removed, etc).
			 *  The argument will be a dict containing the type, name
			 *  and namespace of the changed collection.
			 *
			 *  @param slot Function pointer to a function taking a
			 *              const Dict& and returning a bool.
			 *  @param error Function pointer to an error callback
			 *               function. (<b>optional</b>)
			 *
			 *  @throw connection_error If the client isn't connected.
			 */
			void
			broadcastCollectionChanged( const DictSlot& slot,
			                            const ErrorSlot& error = &Xmms::dummy_error
			                          ) const;


		/** @cond */
		private:

			// Constructor, only to be called by Xmms::Client
			friend class Client;
			Collection( xmmsc_connection_t*& conn, bool& connected,
			            MainloopInterface*& ml );

			// Copy-constructor / operator=
			Collection( const Collection& src );
			Collection operator=( const Collection& src ) const;

			xmmsc_connection_t*& conn_;
			bool& connected_;
			MainloopInterface*& ml_;
		/** @endcond */

	};

}

#endif // XMMSCLIENTPP_COLLECTION_H