--  (C) 2011-16 Nicola Bonelli <nicola@pfq.io>
--
--  This program is free software; you can redistribute it and/or modify
--  it under the terms of the GNU General Public License as published by
--  the Free Software Foundation; either version 2 of the License, or
--  (at your option) any later version.
--
--  This program is distributed in the hope that it will be useful,
--  but WITHOUT ANY WARRANTY; without even the implied warranty of
--  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
--  GNU General Public License for more details.
--
--  You should have received a copy of the GNU General Public License
--  along with this program; if not, write to the Free Software Foundation,
--  Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
--
--  The full GNU General Public License is included in this distribution in
--  the file called "COPYING".

module Main where

import Network.PFQ as Q
import Foreign
import System.Environment

import Network.PFQ.Lang
import Network.PFQ.Lang.Default

handler :: Q.Callback
handler h _ = print h

recvDispatch :: PfqHandlePtr -> IO()
recvDispatch q = do
        Q.dispatch q handler 1000
        -- cs <- Q.getGroupId q >>= Q.getGroupCounters q
        -- print cs
        recvDispatch q

dumper :: String -> IO ()
dumper dev = do
    putStrLn  $ "dumping " ++ dev  ++ "..."
    fp <- Q.open 64 4096 4096
    withPfq fp  $ \q -> do
        Q.timestampingEnable q True
        gid <- Q.getGroupId q
        Q.bindGroup q gid dev (-1)
        Q.enable q
        Q.getRxSlotSize q >>= \o -> putStrLn $ "slot_size: " ++ show o
        recvDispatch q

main :: IO ()
main = do
    args <- getArgs
    case length args of
        0   -> error "usage: test-dispatch dev"
        _   -> dumper (head args)
