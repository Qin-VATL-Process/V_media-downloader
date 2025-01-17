/*
 *
 *  Copyright (c) 2021
 *  name : Francis Banyikwa
 *  email: mhogomchungu@gmail.com
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef NETWORK_ACCESS_H
#define NETWORK_ACCESS_H

#include <memory>

#include <QFile>
#include <QStringList>

#include "engines.h"
#include "utils/network_access_manager.hpp"

class basicdownloader ;
class Context ;
class tabManager ;

class networkAccess
{
public:
	struct iter
	{
		virtual ~iter() ;
		virtual const engines::engine& engine() = 0 ;
		virtual bool hasNext() = 0 ;
		virtual void moveToNext() = 0 ;
		virtual void reportDone() = 0 ;
		virtual const QString& setDefaultEngine() = 0 ;
		virtual const engines::Iterator& itr() = 0 ;
	} ;

	class iterator
	{
	public:
		template< typename Type,typename ... Args >
		iterator( Type,Args&& ... args ) :
			m_handle( std::make_unique< typename Type::type >( std::forward< Args >( args ) ... ) )
		{
		}
		iterator()
		{
		}
		bool hasNext() const
		{
			return m_handle->hasNext() ;
		}
		networkAccess::iterator next() const
		{
			auto m = this->move() ;

			m.m_handle->moveToNext() ;

			return m ;
		}
		networkAccess::iterator move() const
		{
			return std::move( *const_cast< networkAccess::iterator * >( this ) ) ;
		}
		const engines::engine& engine() const
		{
			return m_handle->engine() ;
		}
		void reportDone() const
		{
			m_handle->reportDone() ;
		}
		const QString& setDefaultEngine()
		{
			return m_handle->setDefaultEngine() ;
		}
		const engines::Iterator& itr() const
		{
			return m_handle->itr() ;
		}
	private:
		std::unique_ptr< networkAccess::iter > m_handle ;
	} ;

	networkAccess( const Context& ) ;

	static bool hasNetworkSupport()
	{
		#if QT_VERSION >= QT_VERSION_CHECK( 5,6,0 )
			return true ;
		#else
			return false ;
		#endif
	}
	struct showVersionInfo
	{
		bool show ;
		bool setAfterDownloading ;
	};
	void download( engines::Iterator iter,networkAccess::showVersionInfo v ) const
	{
		class meaw : public networkAccess::iter
		{
		public:
			meaw( engines::Iterator m ) : m_iter( std::move( m ) )
			{
			}
			const engines::engine& engine() override
			{
				return m_iter.engine() ;
			}
			bool hasNext() override
			{
				return m_iter.hasNext() ;
			}
			void moveToNext() override
			{
				m_iter = m_iter.next() ;
			}
			void reportDone() override
			{
			}
			const QString& setDefaultEngine() override
			{
				return m_defaultEngineName ;
			}
			const engines::Iterator& itr() override
			{
				return m_iter ;
			}
		private:
			QString m_defaultEngineName ;
			engines::Iterator m_iter ;
		};

		this->download( { util::types::type_identity< meaw >(),std::move( iter ) },v ) ;
	}
	void download( networkAccess::iterator iter,
		       networkAccess::showVersionInfo showVinfo ) const
	{
		const_cast< networkAccess * >( this )->download( std::move( iter ),showVinfo ) ;
	}
	template< typename Function >
	void get( const QString& url,Function function ) const
	{
		const_cast< networkAccess * >( this )->get( url,std::move( function ) ) ;
	}
private:
	void download( networkAccess::iterator,
		       networkAccess::showVersionInfo ) ;

	template< typename Function >
	void get( const QString& url,Function&& function )
	{
		m_network.get( this->networkRequest( url ),[ function = std::move( function ) ]( const utils::network::reply& reply ){

			if( reply.success() ){

				function( reply.data() ) ;
			}else{
				function( {} ) ;
			}
		} ) ;
	}
	QNetworkRequest networkRequest( const QString& url ) ;
	struct metadata
	{
		qint64 size ;
		QString url ;
		QString fileName ;
	};

	struct Opts
	{
		Opts( networkAccess::iterator itr,
		      const QString& exePath,
		      const QString& efp,
		      const QString& sde,
		      int xd,
		      networkAccess::showVersionInfo svf ) :
			iter( std::move( itr ) ),
			exeBinPath( exePath ),
			tempPath( efp ),
			defaultEngine( sde ),
			showVinfo( svf ),
			id( xd )
		{
		}
		void add( networkAccess::metadata&& m )
		{
			metadata = std::move( m ) ;

			filePath = tempPath + "/" + metadata.fileName ;

			isArchive = filePath.endsWith( ".zip" ) || filePath.contains( ".tar." ) ;

			if( !isArchive ){

				filePath += ".tmp" ;
			}
		}
		void openFile()
		{
			m_file = std::make_shared< QFile >( this->filePath ) ;
			m_file->remove() ;
			m_file->open( QIODevice::WriteOnly ) ;
		}
		QFile& file()
		{
			return *m_file ;
		}
		networkAccess::iterator iter ;
		QString exeBinPath ;
		networkAccess::metadata metadata ;
		QString filePath ;
		QString tempPath ;
		QString defaultEngine ;
		QString networkError ;
		/*
		 * We are using make_shared because old versions of gcc do not work with
		 * unique_ptr when moving the class to lambda capture area
		 */
		std::shared_ptr< QFile > m_file ;
		bool isArchive ;
		networkAccess::showVersionInfo showVinfo ;
		int id ;
	};

	QString downloadFailed() ;

	void extractArchive( const engines::engine&,networkAccess::Opts ) ;

	void download( networkAccess::Opts ) ;

	void download( const QByteArray&,const engines::engine&,networkAccess::Opts ) ;

	void finished( networkAccess::Opts ) ;

	void post( const engines::engine&,const QString&,int ) ;

	const Context& m_ctx ;
	utils::network::manager m_network ;
	basicdownloader& m_basicdownloader ;
	tabManager& m_tabManager ;
};

#endif
