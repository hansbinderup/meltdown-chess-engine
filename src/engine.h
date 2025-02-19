#pragma once

#include "attack_generation.h"
#include "board_defs.h"
#include "file_logger.h"

#include "magic_enum/magic_enum.hpp"
#include "movement/move_types.h"
#include "src/bit_board.h"
#include "src/evaluation/move_scoring.h"
#include "src/movement/move_generation.h"
#include "src/positioning.h"

#include <numeric>

class Engine {
public:
    Engine(FileLogger& logger)
        : m_logger(logger)
    {
        m_bitBoard.reset();
    }

    void reset()
    {
        m_bitBoard.reset();
    }

    BitBoard board() const
    {
        return m_bitBoard;
    }

    uint16_t getRoundsCount() const
    {
        return m_bitBoard.roundsCount;
    }

    movement::ValidMoves getAllMoves() const
    {
        movement::ValidMoves validMoves;

        if (m_bitBoard.player == Player::White) {
            uint64_t attacks = getAllAttacks(Player::Black);
            gen::getKingMoves(validMoves, m_bitBoard, attacks);
            gen::getPawnMoves(validMoves, m_bitBoard);
            gen::getKnightMoves(validMoves, m_bitBoard);
            gen::getRookMoves(validMoves, m_bitBoard);
            gen::getBishopMoves(validMoves, m_bitBoard);
            gen::getQueenMoves(validMoves, m_bitBoard);
            gen::getCastlingMoves(validMoves, m_bitBoard, attacks);
        } else {
            uint64_t attacks = getAllAttacks(Player::White);
            gen::getKingMoves(validMoves, m_bitBoard, attacks);
            gen::getPawnMoves(validMoves, m_bitBoard);
            gen::getKnightMoves(validMoves, m_bitBoard);
            gen::getRookMoves(validMoves, m_bitBoard);
            gen::getBishopMoves(validMoves, m_bitBoard);
            gen::getQueenMoves(validMoves, m_bitBoard);
            gen::getCastlingMoves(validMoves, m_bitBoard, attacks);
        }

        return validMoves;
    }

    movement::ValidMoves getAllMovesSorted(uint8_t ply) const
    {
        auto allMoves = getAllMoves();
        std::sort(allMoves.getMoves().begin(), allMoves.getMoves().end(), [this, ply](const auto& a, const auto& b) {
            return evaluation::MoveScoring::score(m_bitBoard, a, ply) > evaluation::MoveScoring::score(m_bitBoard, b, ply);
        });

        return allMoves;
    }

    movement::ValidMoves getAllCaptures() const
    {
        movement::ValidMoves captures;
        const auto allMoves = getAllMoves();

        for (const auto& move : allMoves.getMoves()) {
            if (move.isCapture())
                captures.addMove(move);
        }

        return captures;
    }

    movement::ValidMoves getAllCapturesSorted(uint8_t ply) const
    {
        auto captures = getAllCaptures();
        std::sort(captures.getMoves().begin(), captures.getMoves().end(), [this, ply](const auto& a, const auto& b) {
            return evaluation::MoveScoring::score(m_bitBoard, a, ply) > evaluation::MoveScoring::score(m_bitBoard, b, ply);
        });

        return captures;
    }

    /*
     * Score in favour of current player
     */
    constexpr int16_t getScore() const
    {
        int16_t score = 0;

        // Material scoring
        score += std::popcount(m_bitBoard.whitePawns) * 100;
        score += std::popcount(m_bitBoard.whiteKnights) * 300;
        score += std::popcount(m_bitBoard.whiteBishops) * 350;
        score += std::popcount(m_bitBoard.whiteRooks) * 500;
        score += std::popcount(m_bitBoard.whiteQueens) * 1000;
        score += std::popcount(m_bitBoard.whiteKing) * 10000;

        score -= std::popcount(m_bitBoard.blackPawns) * 100;
        score -= std::popcount(m_bitBoard.blackKnights) * 300;
        score -= std::popcount(m_bitBoard.blackBishops) * 350;
        score -= std::popcount(m_bitBoard.blackRooks) * 500;
        score -= std::popcount(m_bitBoard.blackQueens) * 1000;
        score -= std::popcount(m_bitBoard.blackKing) * 10000;

        // Positional scoring
        score += positioning::calculatePawnScore(m_bitBoard.whitePawns, Player::White);
        score += positioning::calculateRookScore(m_bitBoard.whiteRooks, Player::White);
        score += positioning::calculateBishopScore(m_bitBoard.whiteBishops, Player::White);
        score += positioning::calculateKnightScore(m_bitBoard.whiteKnights, Player::White);
        score += positioning::calculateQueenScore(m_bitBoard.whiteQueens, Player::White);
        score += positioning::calculateKingScore(m_bitBoard.whiteKing, Player::White);

        score -= positioning::calculatePawnScore(m_bitBoard.blackPawns, Player::Black);
        score -= positioning::calculateRookScore(m_bitBoard.blackRooks, Player::Black);
        score -= positioning::calculateBishopScore(m_bitBoard.blackBishops, Player::Black);
        score -= positioning::calculateKnightScore(m_bitBoard.blackKnights, Player::Black);
        score -= positioning::calculateQueenScore(m_bitBoard.blackQueens, Player::Black);
        score -= positioning::calculateKingScore(m_bitBoard.blackKing, Player::Black);

        return getCurrentPlayer() == Player::White ? score : -score;
    }

    void performMove(const movement::Move& move)
    {
        const auto fromSquare = move.fromSquare();
        const auto toSquare = move.toSquare();

        if (move.isCastleMove()) {
            performCastleMove(move);
        } else {
            if (move.isPromotionMove())
                performPromotionMove(move);
            else {
                // should be done before moving pieces
                updateCastlingRights(fromSquare);
            }

            bitToggleMove(m_bitBoard.whitePawns, fromSquare, toSquare);
            bitToggleMove(m_bitBoard.whiteRooks, fromSquare, toSquare);
            bitToggleMove(m_bitBoard.whiteBishops, fromSquare, toSquare);
            bitToggleMove(m_bitBoard.whiteKnights, fromSquare, toSquare);
            bitToggleMove(m_bitBoard.whiteQueens, fromSquare, toSquare);
            bitToggleMove(m_bitBoard.whiteKing, fromSquare, toSquare);

            bitToggleMove(m_bitBoard.blackPawns, fromSquare, toSquare);
            bitToggleMove(m_bitBoard.blackRooks, fromSquare, toSquare);
            bitToggleMove(m_bitBoard.blackBishops, fromSquare, toSquare);
            bitToggleMove(m_bitBoard.blackKnights, fromSquare, toSquare);
            bitToggleMove(m_bitBoard.blackQueens, fromSquare, toSquare);
            bitToggleMove(m_bitBoard.blackKing, fromSquare, toSquare);
        }

        m_bitBoard.player = nextPlayer(m_bitBoard.player);
        m_bitBoard.roundsCount++;
    }

    void printBoardDebug() const
    {
        m_logger.log("\n******* Bitboard debug: *******\n");

        for (uint8_t row = 8; row > 0; row--) {
            m_logger << std::format("-{}- ", row);

            for (uint8_t column = 0; column < 8; column++) {
                uint64_t square = 1ULL << (((row - 1) * 8) + column);

                if (square & m_bitBoard.whitePawns) {
                    m_logger << "WP ";
                } else if (square & m_bitBoard.whiteRooks) {
                    m_logger << "WR ";
                } else if (square & m_bitBoard.whiteBishops) {
                    m_logger << "WB ";
                } else if (square & m_bitBoard.whiteKnights) {
                    m_logger << "WH ";
                } else if (square & m_bitBoard.whiteQueens) {
                    m_logger << "WQ ";
                } else if (square & m_bitBoard.whiteKing) {
                    m_logger << "WK ";
                } else if (square & m_bitBoard.blackPawns) {
                    m_logger << "BP ";
                } else if (square & m_bitBoard.blackRooks) {
                    m_logger << "BR ";
                } else if (square & m_bitBoard.blackBishops) {
                    m_logger << "BB ";
                } else if (square & m_bitBoard.blackKnights) {
                    m_logger << "BH ";
                } else if (square & m_bitBoard.blackQueens) {
                    m_logger << "BQ ";
                } else if (square & m_bitBoard.blackKing) {
                    m_logger << "BK ";
                } else {
                    m_logger << "## ";
                }
            }

            m_logger << "\n";
        }

        m_logger << "    A  B  C  D  E  F  G  H";
        m_logger << "\n\n";

        const auto allMoves = getAllMoves();
        m_logger << "Player: " << magic_enum::enum_name(m_bitBoard.player);
        m_logger << "\nRound: " << std::to_string(m_bitBoard.roundsCount);
        m_logger << "\nCastle: " << std::accumulate(allMoves.getMoves().begin(), allMoves.getMoves().end(), std::string {}, [](std::string result, const movement::Move& move) {
            if (move.isCastleMove()) {
                /* result += move.toString() + " "; */
            }
            return result;
        });

        m_logger << std::format("\n\nMoves[{}]:\n", allMoves.count());
        for (const auto& move : allMoves.getMoves()) {
            m_logger << std::format("{} [{}]  ", move.toString().data(), evaluation::MoveScoring::score(m_bitBoard, move, 0));
        }

        const auto captures = getAllCapturesSorted(0);
        m_logger << std::format("\n\nCaptures[{}]:\n", captures.count());
        for (const auto& move : captures.getMoves()) {
            m_logger << std::format("{} [{}]  ", move.toString().data(), evaluation::MoveScoring::score(m_bitBoard, move, 0));
        }

        m_logger << "\n\n";
        m_logger.flush();
    }

    Player getCurrentPlayer() const
    {
        return m_bitBoard.player;
    }

    constexpr uint64_t getAllAttacks(Player player) const
    {
        uint64_t attacks = player == Player::White
            ? gen::getWhitePawnAttacks(m_bitBoard)
            : gen::getBlackPawnAttacks(m_bitBoard);

        attacks |= gen::getKnightAttacks(m_bitBoard, player);
        attacks |= gen::getRookAttacks(m_bitBoard, player);
        attacks |= gen::getBishopAttacks(m_bitBoard, player);
        attacks |= gen::getQueenAttacks(m_bitBoard, player);
        attacks |= gen::getKingAttacks(m_bitBoard, player);

        return attacks;
    }

    constexpr bool isKingAttacked(Player player) const
    {
        if (player == Player::White) {
            return m_bitBoard.whiteKing & getAllAttacks(Player::Black);
        } else {
            return m_bitBoard.blackKing & getAllAttacks(Player::White);
        }
    }

    constexpr bool isKingAttacked() const
    {
        if (m_bitBoard.player == Player::White) {
            return m_bitBoard.whiteKing & getAllAttacks(Player::Black);
        } else {
            return m_bitBoard.blackKing & getAllAttacks(Player::White);
        }
    }

    constexpr void performPromotionMove(const movement::Move& move)
    {
        const bool isWhite = m_bitBoard.player == Player::White;

        // first clear to be promoted pawn
        uint64_t& pawns = isWhite ? m_bitBoard.whitePawns : m_bitBoard.blackPawns;
        pawns &= ~move.fromSquare();

        const auto findPromotionPiece = [&] -> uint64_t& {
            switch (move.promotionType()) {
            case PromotionType::Queen:
            case PromotionType::None:
                return isWhite ? m_bitBoard.whiteQueens : m_bitBoard.blackQueens;
            case PromotionType::Knight:
                return isWhite ? m_bitBoard.whiteKnights : m_bitBoard.blackKnights;
            case PromotionType::Bishop:
                return isWhite ? m_bitBoard.whiteBishops : m_bitBoard.blackBishops;
            case PromotionType::Rook:
                return isWhite ? m_bitBoard.whiteRooks : m_bitBoard.blackRooks;
            }

            return isWhite ? m_bitBoard.whiteQueens : m_bitBoard.blackQueens;
        };

        // replace pawn with given promotion type
        uint64_t& promotionPiece = findPromotionPiece();
        promotionPiece |= move.fromSquare();
    }

    constexpr void performCastleMove(const movement::Move& move)
    {
        const auto fromSquare = move.fromSquare();
        const auto toSquare = move.toSquare();

        switch (move.castleType()) {
        case CastleType::WhiteKingSide: {
            bitToggleMove(m_bitBoard.whiteKing, fromSquare, toSquare);
            bitToggleMove(m_bitBoard.whiteRooks, 1ULL << gen::s_whiteKingSideCastleMoveRook.first, 1ULL << gen::s_whiteKingSideCastleMoveRook.second);
            m_bitBoard.whiteCastlingRights = 0;
        } break;

        case CastleType::WhiteQueenSide: {
            bitToggleMove(m_bitBoard.whiteKing, fromSquare, toSquare);
            bitToggleMove(m_bitBoard.whiteRooks, 1ULL << gen::s_whiteQueenSideCastleMoveRook.first, 1ULL << gen::s_whiteQueenSideCastleMoveRook.second);
            m_bitBoard.whiteCastlingRights = 0;
        } break;
        case CastleType::BlackKingSide: {
            bitToggleMove(m_bitBoard.blackKing, fromSquare, toSquare);
            bitToggleMove(m_bitBoard.blackRooks, 1ULL << gen::s_blackKingSideCastleMoveRook.first, 1ULL << gen::s_blackKingSideCastleMoveRook.second);
            m_bitBoard.blackCastlingRights = 0;
        } break;
        case CastleType::BlackQueenSide: {
            bitToggleMove(m_bitBoard.blackKing, fromSquare, toSquare);
            bitToggleMove(m_bitBoard.blackRooks, 1ULL << gen::s_blackQueenSideCastleMoveRook.first, 1ULL << gen::s_blackQueenSideCastleMoveRook.second);
            m_bitBoard.blackCastlingRights = 0;
        } break;
        case CastleType::None:
            break;
        }
    }

    constexpr void updateCastlingRights(uint64_t fromSquare)
    {
        if (m_bitBoard.player == Player::White && m_bitBoard.whiteCastlingRights) {
            if (fromSquare & m_bitBoard.whiteKing) {
                m_bitBoard.whiteCastlingRights = 0;
            } else {
                m_bitBoard.whiteCastlingRights &= ~fromSquare;
            }
        } else if (m_bitBoard.player == Player::Black && m_bitBoard.blackCastlingRights) {
            if (fromSquare & m_bitBoard.blackKing) {
                m_bitBoard.blackCastlingRights = 0;
            } else {
                m_bitBoard.blackCastlingRights &= ~fromSquare;
            }
        }
    }

    constexpr void bitToggleMove(uint64_t& piece, uint64_t fromSquare, uint64_t toSquare)
    {
        if (toSquare & piece) {
            // clear piece if being attacked
            piece &= ~toSquare;
        } else if (fromSquare & piece) {
            // move if given piece is located at "fromSquare"
            piece &= ~fromSquare;
            piece |= toSquare;
        }
    }

private:
    BitBoard m_bitBoard;
    FileLogger& m_logger;
};
