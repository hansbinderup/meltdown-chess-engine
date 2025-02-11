#pragma once

#include "attack_generation.h"
#include "board_defs.h"
#include "file_logger.h"

#include "magic_enum/magic_enum.hpp"
#include "magic_enum/magic_enum_flags.hpp"
#include "movement/move_types.h"
#include "src/bit_board.h"
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

    movement::ValidMoves getAllMovesSorted() const
    {
        auto allMoves = getAllMoves();
        std::sort(allMoves.getMoves().begin(), allMoves.getMoves().end(), [this](const auto& a, const auto& b) {
            return scoreMove(a) > scoreMove(b);
        });

        return allMoves;
    }

    movement::ValidMoves getAllCaptures() const
    {
        movement::ValidMoves captures;
        const auto allMoves = getAllMoves();

        for (const auto& move : allMoves.getMoves()) {
            if (magic_enum::enum_flags_test(move.flags, movement::MoveFlags::Capture))
                captures.addMove(move);
        }

        return captures;
    }

    movement::ValidMoves getAllCapturesSorted() const
    {
        auto captures = getAllCaptures();
        std::sort(captures.getMoves().begin(), captures.getMoves().end(), [this](const auto& a, const auto& b) {
            return scoreMove(a) > scoreMove(b);
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
        const auto moveFlags = move.flags;

        if (magic_enum::enum_flags_test(moveFlags, movement::MoveFlags::Castle)) {
            performCastleMove(move);
        } else {
            // should be done before moving pieces
            updateCastlingRights(fromSquare);

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
            if (magic_enum::enum_flags_test(move.flags, movement::MoveFlags::Castle)) {
                result += move.toString() + " ";
            }
            return result;
        });

        m_logger << std::format("\n\nMoves[{}]:\n", allMoves.count());
        for (const auto& move : allMoves.getMoves()) {
            m_logger << std::format("{} [{}]  ", move.toString(), scoreMove(move));
        }

        const auto captures = getAllCapturesSorted();
        m_logger << std::format("\n\nCaptures[{}]:\n", captures.count());
        for (const auto& move : captures.getMoves()) {
            m_logger << std::format("{} [{}]  ", move.toString(), scoreMove(move));
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

    /*
     * Could be made better I guess, but for now this should do
     */
    constexpr void performCastleMove(const movement::Move& move)
    {
        const auto fromSquare = move.fromSquare();
        const auto toSquare = move.toSquare();

        auto performSingleCastleMove = [&](uint64_t& king, uint64_t& rooks, uint64_t& castlingRights,
                                           const movement::Move& queenSideMove, const movement::Move& kingSideMove,
                                           const movement::Move& queenSideRookMove, const movement::Move& kingSideRookMove) {
            if (move == queenSideMove) {
                bitToggleMove(king, fromSquare, toSquare);
                bitToggleMove(rooks, 1ULL << queenSideRookMove.from, 1ULL << queenSideRookMove.to);
                castlingRights = 0;
            }
            if (move == kingSideMove) {
                bitToggleMove(king, fromSquare, toSquare);
                bitToggleMove(rooks, 1ULL << kingSideRookMove.from, 1ULL << kingSideRookMove.to);
                castlingRights = 0;
            }
        };

        if (m_bitBoard.player == Player::White && m_bitBoard.whiteCastlingRights) {
            performSingleCastleMove(m_bitBoard.whiteKing, m_bitBoard.whiteRooks, m_bitBoard.whiteCastlingRights,
                gen::s_whiteQueenSideCastleMove, gen::s_whiteKingSideCastleMove,
                gen::s_whiteQueenSideCastleMoveRook, gen::s_whiteKingSideCastleMoveRook);
        }

        if (m_bitBoard.player == Player::Black && m_bitBoard.blackCastlingRights) {
            performSingleCastleMove(m_bitBoard.blackKing, m_bitBoard.blackRooks, m_bitBoard.blackCastlingRights,
                gen::s_blackQueenSideCastleMove, gen::s_blackKingSideCastleMove,
                gen::s_blackQueenSideCastleMoveRook, gen::s_blackKingSideCastleMoveRook);
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

    constexpr int16_t scoreMove(const movement::Move& move) const
    {
        if (!magic_enum::enum_flags_test(move.flags, movement::MoveFlags::Capture)) {
            return 0;
        }

        const auto attacker = m_bitBoard.getPieceAtSquare(move.fromSquare());
        const auto victim = m_bitBoard.getPieceAtSquare(move.toSquare());

        if (attacker.has_value() && victim.has_value()) {
            return gen::getMvvLvaScore(attacker.value(), victim.value());
        }

        // shouldn't happen
        return 0;
    }

private:
    BitBoard m_bitBoard;
    FileLogger& m_logger;
};
