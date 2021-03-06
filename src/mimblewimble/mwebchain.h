#pragma once

#include <interfaces/chain.h>
#include <primitives/block.h>

#include <libmw/libmw.h>
#include <stdexcept>

class MWChainIterator : public libmw::IChainIterator
{
public:
    MWChainIterator(interfaces::Chain& chain)
        : m_chain(chain), m_locked(chain.lock()), m_height(0) { }

    void SeekToFirstMWEB() noexcept
    {
        while (Valid()) {
            CBlock block;
            if (m_chain.findBlock(m_locked->getBlockHash(m_height), &block) && !block.mwBlock.IsNull()) {
                break;
            }

            Next();
        }
    }

    void Next() noexcept final { ++m_height; }
    bool Valid() const noexcept final { return m_locked->getHeight() != nullopt && m_height <= *m_locked->getHeight(); }

    uint64_t GetHeight() const final { return m_height; }
    libmw::BlockHash GetCanonicalHash() const final
    {
        uint256 hash256 = m_locked->getBlockHash(m_height);
        libmw::BlockHash block_hash;
        std::copy_n(hash256.begin(), 32, block_hash.begin());
        return block_hash;
    }

    libmw::HeaderRef GetHeader() const final
    {
        CBlock block;
        if (m_chain.findBlock(m_locked->getBlockHash(m_height), &block) && !block.mwBlock.IsNull()) {
            return block.mwBlock.m_block.GetHeader();
        }

        throw std::runtime_error("MWEB header not found");
    }

    libmw::HeaderAndPegsRef GetHeaderAndPegs() const final
    {
        CBlock block;
        if (m_chain.findBlock(m_locked->getBlockHash(m_height), &block) && !block.mwBlock.IsNull()) {
            libmw::HeaderRef header = block.mwBlock.m_block.GetHeader();

            return libmw::HeaderAndPegsRef{header, block.GetPegInCoins(), block.GetPegOutCoins()};
        }

        throw std::runtime_error("MWEB block not found");
    }

    libmw::BlockRef GetBlock() const final
    {
        CBlock block;
        if (m_chain.findBlock(m_locked->getBlockHash(m_height), &block) && !block.mwBlock.IsNull()) {
            return block.mwBlock.m_block;
        }

        throw std::runtime_error("MWEB block not found");
    }

private:
    interfaces::Chain& m_chain;
    std::unique_ptr<interfaces::Chain::Lock> m_locked;
    int m_height;
};

class MWChain : public libmw::IChain
{
public:
    MWChain(interfaces::Chain& chain)
        : m_chain(chain) { }

    std::unique_ptr<libmw::IChainIterator> NewIterator() final
    {
        auto pIter = new MWChainIterator(m_chain);
        pIter->SeekToFirstMWEB();
        return std::unique_ptr<libmw::IChainIterator>(pIter);
    }

private:
    interfaces::Chain& m_chain;
};