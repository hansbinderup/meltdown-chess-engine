#pragma once

/* ===========================================================
   Auto-Generated Tuning Parameters
   -----------------------------------------------------------
   Generated: 09/04/2025 at 14:46
   -----------------------------------------------------------
   Parameters:
     ▸ K-value          = 1.50
     ▸ Learning Rate    = 1.00
     ▸ LR Step Rate     = 250
     ▸ LR Drop Rate     = 2.00
     ▸ Epochs           = 2000
   -----------------------------------------------------------
   See the 'tuner/' for more information on how to generate
   =========================================================== */

#include "evaluation/terms.h"

namespace evaluation {

constexpr Terms s_terms = {
    .pieceValues = {
        Score(104, 115),
        Score(294, 246),
        Score(270, 238),
        Score(348, 394),
        Score(594, 618),
        Score(0, 0),
    },
    .doublePawnPenalty = {
        Score(-13, -20),
    },
    .isolatedPawnPenalty = {
        Score(-30, -18),
    },
    .passedPawnBonus = {
        Score(0, 0),
        Score(0, 9),
        Score(-14, 21),
        Score(-10, 64),
        Score(35, 104),
        Score(41, 197),
        Score(121, 227),
        Score(0, 0),
    },
    .pawnShieldBonus = {
        Score(77, -29),
        Score(62, -17),
        Score(44, -31),
    },
    .rookOpenFileBonus = {
        Score(39, -5),
    },
    .rookSemiOpenFileBonus = {
        Score(33, 31),
    },
    .queenOpenFileBonus = {
        Score(-72, 108),
    },
    .queenSemiOpenFileBonus = {
        Score(-21, 184),
    },
    .knightMobilityScore = {
        Score(151, 135),
        Score(184, 166),
        Score(197, 183),
        Score(203, 178),
        Score(209, 192),
        Score(207, 200),
        Score(203, 205),
        Score(201, 208),
        Score(202, 202),
    },
    .bishopMobilityScore = {
        Score(0, 0),
        Score(72, 52),
        Score(113, 65),
        Score(126, 121),
        Score(158, 137),
        Score(171, 158),
        Score(200, 186),
        Score(217, 200),
        Score(233, 217),
        Score(236, 226),
        Score(249, 234),
        Score(250, 232),
        Score(246, 230),
        Score(266, 234),
    },
    .rookMobilityScore = {
        Score(0, 0),
        Score(0, 0),
        Score(221, 251),
        Score(238, 284),
        Score(251, 304),
        Score(258, 312),
        Score(261, 319),
        Score(264, 330),
        Score(272, 332),
        Score(280, 337),
        Score(286, 345),
        Score(291, 349),
        Score(293, 356),
        Score(301, 358),
        Score(292, 359),
    },
    .queenMobilityScore = {
        Score(0, 0),
        Score(0, 0),
        Score(0, 0),
        Score(67, 24),
        Score(297, 144),
        Score(444, 199),
        Score(463, 298),
        Score(461, 436),
        Score(471, 487),
        Score(470, 532),
        Score(480, 548),
        Score(486, 566),
        Score(492, 584),
        Score(499, 583),
        Score(509, 584),
        Score(514, 585),
        Score(519, 584),
        Score(527, 580),
        Score(532, 574),
        Score(538, 569),
        Score(548, 567),
        Score(566, 541),
        Score(563, 550),
        Score(575, 531),
        Score(560, 528),
        Score(542, 531),
        Score(452, 516),
        Score(342, 501),
    },
    .bishopPairScore = {
        Score(70, 99),
    },
    .psqtPawns = {
        Score(0, 0),
        Score(0, 0),
        Score(0, 0),
        Score(0, 0),
        Score(0, 0),
        Score(0, 0),
        Score(0, 0),
        Score(0, 0),
        Score(11, 105),
        Score(24, 100),
        Score(32, 87),
        Score(30, 90),
        Score(58, 104),
        Score(58, 95),
        Score(79, 76),
        Score(1, 77),
        Score(11, 94),
        Score(21, 91),
        Score(51, 74),
        Score(55, 75),
        Score(83, 78),
        Score(46, 83),
        Score(64, 74),
        Score(28, 67),
        Score(8, 105),
        Score(30, 101),
        Score(56, 73),
        Score(87, 62),
        Score(89, 60),
        Score(67, 76),
        Score(36, 86),
        Score(16, 74),
        Score(35, 137),
        Score(50, 119),
        Score(72, 87),
        Score(75, 57),
        Score(119, 55),
        Score(88, 76),
        Score(64, 105),
        Score(44, 101),
        Score(71, 180),
        Score(77, 187),
        Score(139, 108),
        Score(130, 51),
        Score(146, 42),
        Score(204, 76),
        Score(136, 144),
        Score(68, 152),
        Score(114, 225),
        Score(120, 189),
        Score(109, 196),
        Score(165, 104),
        Score(112, 105),
        Score(69, 123),
        Score(-89, 198),
        Score(-124, 232),
        Score(0, 0),
        Score(0, 0),
        Score(0, 0),
        Score(0, 0),
        Score(0, 0),
        Score(0, 0),
        Score(0, 0),
        Score(0, 0),
    },
    .psqtKnights = {
        Score(-69, 87),
        Score(50, 83),
        Score(20, 113),
        Score(54, 118),
        Score(70, 111),
        Score(85, 91),
        Score(58, 103),
        Score(-14, 102),
        Score(16, 112),
        Score(37, 126),
        Score(76, 126),
        Score(121, 118),
        Score(117, 124),
        Score(117, 116),
        Score(82, 101),
        Score(88, 110),
        Score(28, 110),
        Score(85, 126),
        Score(126, 138),
        Score(144, 166),
        Score(175, 155),
        Score(144, 125),
        Score(135, 114),
        Score(73, 113),
        Score(68, 138),
        Score(99, 149),
        Score(154, 174),
        Score(163, 173),
        Score(181, 180),
        Score(164, 158),
        Score(153, 137),
        Score(100, 114),
        Score(93, 123),
        Score(128, 151),
        Score(182, 175),
        Score(236, 170),
        Score(196, 174),
        Score(244, 162),
        Score(156, 148),
        Score(166, 112),
        Score(77, 121),
        Score(165, 127),
        Score(214, 150),
        Score(233, 150),
        Score(291, 124),
        Score(295, 116),
        Score(216, 105),
        Score(144, 94),
        Score(40, 106),
        Score(88, 118),
        Score(145, 123),
        Score(172, 126),
        Score(134, 113),
        Score(257, 83),
        Score(85, 113),
        Score(139, 66),
        Score(-124, -4),
        Score(-48, 96),
        Score(11, 128),
        Score(68, 103),
        Score(93, 123),
        Score(23, 74),
        Score(5, 94),
        Score(-38, -43),
    },
    .psqtBishops = {
        Score(162, 159),
        Score(198, 170),
        Score(167, 153),
        Score(137, 159),
        Score(154, 154),
        Score(154, 177),
        Score(173, 147),
        Score(193, 127),
        Score(184, 184),
        Score(175, 148),
        Score(185, 132),
        Score(145, 153),
        Score(160, 155),
        Score(182, 138),
        Score(217, 151),
        Score(188, 147),
        Score(158, 173),
        Score(164, 164),
        Score(159, 163),
        Score(156, 165),
        Score(162, 170),
        Score(162, 158),
        Score(171, 151),
        Score(191, 158),
        Score(141, 171),
        Score(132, 174),
        Score(137, 174),
        Score(185, 163),
        Score(177, 163),
        Score(141, 162),
        Score(138, 165),
        Score(169, 153),
        Score(139, 180),
        Score(150, 173),
        Score(169, 158),
        Score(203, 171),
        Score(190, 159),
        Score(178, 163),
        Score(156, 164),
        Score(147, 177),
        Score(151, 187),
        Score(174, 162),
        Score(168, 157),
        Score(197, 137),
        Score(183, 143),
        Score(244, 153),
        Score(219, 153),
        Score(209, 176),
        Score(124, 153),
        Score(142, 153),
        Score(131, 159),
        Score(113, 161),
        Score(164, 140),
        Score(160, 144),
        Score(145, 157),
        Score(147, 151),
        Score(134, 171),
        Score(70, 190),
        Score(76, 182),
        Score(58, 187),
        Score(72, 183),
        Score(76, 165),
        Score(73, 180),
        Score(88, 163),
    },
    .psqtRooks = {
        Score(171, 294),
        Score(167, 282),
        Score(173, 290),
        Score(187, 275),
        Score(201, 262),
        Score(203, 275),
        Score(210, 260),
        Score(187, 271),
        Score(126, 282),
        Score(132, 283),
        Score(156, 277),
        Score(158, 275),
        Score(170, 259),
        Score(198, 243),
        Score(229, 229),
        Score(159, 254),
        Score(123, 293),
        Score(120, 288),
        Score(134, 280),
        Score(138, 282),
        Score(164, 266),
        Score(178, 254),
        Score(242, 220),
        Score(198, 239),
        Score(121, 306),
        Score(115, 308),
        Score(130, 304),
        Score(146, 294),
        Score(151, 289),
        Score(152, 285),
        Score(195, 272),
        Score(173, 271),
        Score(143, 319),
        Score(156, 309),
        Score(158, 318),
        Score(167, 305),
        Score(174, 284),
        Score(205, 278),
        Score(221, 282),
        Score(214, 279),
        Score(151, 317),
        Score(194, 314),
        Score(185, 313),
        Score(183, 304),
        Score(237, 285),
        Score(263, 275),
        Score(327, 272),
        Score(278, 267),
        Score(179, 317),
        Score(174, 333),
        Score(206, 336),
        Score(231, 316),
        Score(210, 316),
        Score(275, 298),
        Score(255, 300),
        Score(296, 282),
        Score(190, 319),
        Score(148, 334),
        Score(155, 344),
        Score(148, 337),
        Score(167, 327),
        Score(180, 327),
        Score(179, 329),
        Score(244, 310),
    },
    .psqtQueens = {
        Score(405, 455),
        Score(409, 445),
        Score(429, 417),
        Score(451, 450),
        Score(442, 404),
        Score(394, 446),
        Score(418, 433),
        Score(413, 415),
        Score(428, 429),
        Score(420, 457),
        Score(445, 436),
        Score(450, 448),
        Score(441, 464),
        Score(457, 413),
        Score(462, 395),
        Score(482, 363),
        Score(411, 454),
        Score(411, 516),
        Score(414, 510),
        Score(412, 506),
        Score(421, 507),
        Score(419, 559),
        Score(447, 529),
        Score(446, 495),
        Score(400, 523),
        Score(396, 530),
        Score(395, 534),
        Score(415, 544),
        Score(413, 557),
        Score(406, 587),
        Score(428, 574),
        Score(447, 555),
        Score(399, 515),
        Score(396, 540),
        Score(414, 522),
        Score(420, 547),
        Score(428, 569),
        Score(439, 594),
        Score(446, 601),
        Score(463, 566),
        Score(425, 513),
        Score(418, 498),
        Score(430, 520),
        Score(455, 529),
        Score(480, 539),
        Score(531, 571),
        Score(534, 544),
        Score(534, 557),
        Score(426, 504),
        Score(378, 534),
        Score(422, 519),
        Score(422, 517),
        Score(446, 538),
        Score(485, 549),
        Score(451, 558),
        Score(539, 544),
        Score(426, 431),
        Score(410, 454),
        Score(440, 475),
        Score(457, 479),
        Score(467, 490),
        Score(468, 487),
        Score(488, 470),
        Score(482, 442),
    },
    .psqtKings = {
        Score(72, -109),
        Score(79, -64),
        Score(59, -35),
        Score(-102, -10),
        Score(7, -38),
        Score(-76, 0),
        Score(51, -42),
        Score(94, -115),
        Score(132, -55),
        Score(31, -2),
        Score(-2, 17),
        Score(-75, 39),
        Score(-75, 47),
        Score(-47, 37),
        Score(64, 0),
        Score(83, -36),
        Score(-6, -39),
        Score(1, 14),
        Score(-112, 57),
        Score(-153, 84),
        Score(-134, 83),
        Score(-125, 67),
        Score(-48, 32),
        Score(-52, 3),
        Score(-67, -27),
        Score(-86, 35),
        Score(-158, 85),
        Score(-247, 121),
        Score(-241, 118),
        Score(-150, 90),
        Score(-147, 64),
        Score(-171, 28),
        Score(-60, -13),
        Score(-92, 58),
        Score(-116, 95),
        Score(-221, 120),
        Score(-198, 121),
        Score(-127, 111),
        Score(-95, 86),
        Score(-125, 33),
        Score(-88, -1),
        Score(60, 49),
        Score(-71, 84),
        Score(-101, 101),
        Score(-48, 105),
        Score(49, 98),
        Score(50, 89),
        Score(4, 26),
        Score(-53, -33),
        Score(-18, 36),
        Score(-65, 53),
        Score(-4, 45),
        Score(-18, 67),
        Score(-6, 85),
        Score(63, 69),
        Score(58, 15),
        Score(-40, -149),
        Score(-45, -71),
        Score(-31, -44),
        Score(-66, 1),
        Score(-44, -12),
        Score(2, 4),
        Score(42, 16),
        Score(19, -127),
    },
};
}