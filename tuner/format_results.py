import re

def parse_parameter_definitions(code):
    """Parses C++ parameter definitions and extracts their name and parameter count."""
    pattern = re.compile(r"(WeightTable<(?P<size>\d+)>|SingleWeight)\s+(?P<name>\w+);")
    
    parameters = {}
    for match in pattern.finditer(code):
        size = int(match.group("size")) if match.group("size") else 1
        name = match.group("name")
        parameters[name] = size
    
    return parameters

def extract_scores(text):
    # Define the regex pattern to match the Score format
    pattern = re.compile(r'Score\(([\d.-]+),\s*([\d.-]+)\)')

    # Find all matches in the text
    matches = pattern.findall(text)

    # Format the scores and round them to the closest integer
    scores = [f"Score({round(float(match[0]))}, {round(float(match[1]))})" for match in matches]

    return scores

def extract_and_assign_scores(text, params):
    # Regex pattern to match Score(x, y) even with breaks and white spaces
    pattern = re.compile(r'Score\(([\d.-]+)\s*,\s*([\d.-]+)\)', re.DOTALL)
    
    # Find all matches in the text
    matches = pattern.findall(text)
    
    # Create a dictionary to store the results
    result = {}
    
    # Flatten the list of matches and round values to integers
    flat_scores = [(round(float(x)), round(float(y))) for x, y in matches]

    # Iterate over each parameter and assign the appropriate number of scores
    score_index = 0
    for param_name, num_scores in params.items():
        if score_index + num_scores <= len(flat_scores):
            result[param_name] = f".{param_name} = {{ " + ", ".join([f"Score({flat_scores[i][0]},{flat_scores[i][1]})" for i in range(score_index, score_index + num_scores)]) + ", },"
            score_index += num_scores
    
    return result

def fix_linebreaks(input_string):
    return input_string.replace("\n", "").replace(" ", "")

# Example C++ definitions
cpp_code = """
    WeightTable<6> pieceValues;
    SingleWeight doublePawnPenalty;
    SingleWeight isolatedPawnPenalty;
    WeightTable<8> passedPawnBonus;
    SingleWeight rookOpenFileBonus;
    SingleWeight rookSemiOpenFileBonus;
    SingleWeight queenOpenFileBonus;
    SingleWeight queenSemiOpenFileBonus;
    WeightTable<9> knightMobilityScore;
    WeightTable<14> bishopMobilityScore;
    WeightTable<15> rookMobilityScore;
    WeightTable<28> queenMobilityScore;
    SingleWeight bishopPairScore;
    SingleWeight kingShieldBonus;
    WeightTable<64> psqtPawns;
    WeightTable<64> psqtKnights;
    WeightTable<64> psqtBishops;
    WeightTable<64> psqtRooks;
    WeightTable<64> psqtQueens;
    WeightTable<64> psqtKings;
"""

scores = """
Score(162.32,176.25) Score(445.07,390.69) Score(465.39,403.07) Score(579.96,624.31) Score(1138.56,1059.43) Score(0.00,0.00) Score(12.67,-39.42) Score(-61.03,-15.20) Score(0.00,0.00) Score(5.83,34.00) Score(8.92,35.45) Score(13.94,96.39) Score(108.43,131.21) Score(181.63,194.21) Score(222.32,246.70) Score(200.00,200.00) Score(62.98,61.15) Score(66.93,68.12) Score(27.78,81.57) Score(51.69,84.12) Score(-84.57,-12.47) Score(-52.68,35.15) Score(2.76,84.76) Score(43.74,91.45) Score(114.37,104.12) Score(102.86,103.40) Score(97.94,102.92) Score(102.95,103.31) Score(108.35,103.39) Score(0.00,0.00) Score(-75.16,-75.93) Score(-91.06,-30.59) Score(-81.33,16.47) Score(-53.45,37.14) Score(3.66,45.14) Score(97.06,65.95) Score(104.24,97.26) Score(105.97,101.78) Score(107.50,106.88) Score(108.16,106.32) Score(107.69,108.21) Score(106.94,104.24) Score(109.81,108.35) Score(0.00,0.00) Score(0.00,0.00) Score(-23.83,79.07) Score(5.78,82.95) Score(35.33,107.55) Score(46.67,116.28) Score(63.20,111.49) Score(80.94,104.38) Score(92.52,126.09) Score(103.99,113.79) Score(105.33,111.86) Score(103.35,110.30) Score(101.37,107.70) Score(100.37,104.56) Score(88.49,99.31) Score(0.00,0.00) Score(0.00,0.00) Score(0.00,0.00) Score(-34.25,-7.49) Score(-86.88,-31.78) Score(-42.80,-6.01) Score(-82.29,-7.87) Score(-61.54,37.46) Score(-19.04,63.22) Score(0.73,80.04) Score(47.42,92.30) Score(81.77,100.48) Score(101.11,114.50) Score(106.38,133.74) Score(113.56,125.59) Score(114.10,122.30) Score(114.57,121.73) Score(115.31,121.27) Score(115.18,120.93) Score(115.92,120.56) Score(116.35,120.67) Score(117.22,121.20) Score(116.86,119.54) Score(116.69,119.39) Score(114.97,118.12) Score(117.20,120.33) Score(113.68,117.45) Score(108.35,115.78) Score(110.20,105.25) Score(55.33,-0.27) Score(0.00,0.00) Score(0.00,0.00) Score(0.00,0.00) Score(0.00,0.00) Score(0.00,0.00) Score(0.00,0.00) Score(0.00,0.00) Score(0.00,0.00) Score(-12.20,107.46) Score(5.83,92.06) Score(-45.85,82.82) Score(-5.02,67.39) Score(59.08,101.37) Score(46.45,73.57) Score(66.10,57.48) Score(-50.53,73.93) Score(-22.46,83.90) Score(-30.50,85.97) Score(5.12,59.68) Score(-24.45,68.64) Score(45.98,65.96) Score(-4.68,70.90) Score(63.04,52.36) Score(41.14,29.17) Score(-36.79,102.42) Score(6.10,97.53) Score(39.80,34.68) Score(107.55,23.34) Score(97.53,22.30) Score(39.09,54.83) Score(-2.30,68.58) Score(-25.38,47.88) Score(53.03,132.51) Score(54.21,110.76) Score(94.37,47.85) Score(97.79,7.66) Score(125.22,39.04) Score(114.45,60.42) Score(59.06,105.94) Score(74.68,108.94) Score(83.75,199.11) Score(100.44,203.04) Score(120.69,140.01) Score(109.77,59.07) Score(142.16,51.52) Score(163.83,129.56) Score(127.41,181.53) Score(57.12,176.66) Score(189.05,282.43) Score(215.07,276.81) Score(139.77,245.81) Score(188.59,177.86) Score(150.76,166.34) Score(111.55,167.89) Score(-24.14,222.84) Score(-130.23,288.62) Score(0.00,0.00) Score(0.00,0.00) Score(0.00,0.00) Score(0.00,0.00) Score(0.00,0.00) Score(0.00,0.00) Score(0.00,0.00) Score(0.00,0.00) Score(-194.06,-72.07) Score(-58.54,7.42) Score(-153.06,-9.09) Score(-117.76,-11.26) Score(-98.86,-46.29) Score(-99.76,-33.69) Score(-7.47,26.37) Score(-124.49,2.20) Score(-123.86,26.82) Score(-139.67,59.00) Score(-105.24,32.09) Score(-28.23,44.55) Score(14.59,50.29) Score(-27.07,25.96) Score(-103.22,-6.67) Score(-60.41,57.10) Score(-121.85,5.15) Score(-93.47,76.73) Score(75.92,59.59) Score(71.06,118.10) Score(96.67,98.45) Score(104.26,24.85) Score(67.72,31.40) Score(-34.46,50.70) Score(-104.93,103.23) Score(11.73,96.26) Score(116.83,115.07) Score(114.99,124.95) Score(137.50,121.76) Score(120.17,99.19) Score(103.32,75.46) Score(-14.00,36.52) Score(28.98,0.73) Score(69.61,91.24) Score(128.50,126.42) Score(164.41,128.56) Score(148.43,128.06) Score(180.55,116.10) Score(107.02,77.22) Score(124.10,38.96) Score(21.75,66.45) Score(153.36,70.88) Score(145.71,113.28) Score(170.79,108.86) Score(191.75,98.53) Score(236.63,88.71) Score(174.03,55.22) Score(122.70,2.63) Score(8.12,58.17) Score(54.97,33.23) Score(164.20,53.45) Score(137.75,81.89) Score(106.89,40.78) Score(167.55,47.35) Score(80.98,7.41) Score(87.19,-5.62) Score(-64.35,33.16) Score(-37.16,43.45) Score(47.06,75.72) Score(45.33,52.98) Score(145.39,51.88) Score(1.49,-5.91) Score(37.45,24.44) Score(-2.55,-4.51) Score(-4.97,64.48) Score(112.25,100.28) Score(69.21,71.02) Score(-65.60,54.04) Score(-55.97,36.13) Score(17.72,84.71) Score(8.01,61.31) Score(-42.46,39.03) Score(121.21,103.84) Score(99.52,90.25) Score(122.54,66.39) Score(14.39,106.68) Score(80.00,101.41) Score(116.63,73.84) Score(151.72,91.96) Score(106.52,79.97) Score(101.89,85.86) Score(103.87,96.80) Score(91.59,101.64) Score(114.07,107.80) Score(112.69,112.76) Score(108.18,88.33) Score(115.17,80.23) Score(126.73,89.12) Score(25.78,98.84) Score(97.56,104.81) Score(110.18,111.77) Score(135.27,121.32) Score(143.00,110.76) Score(104.78,102.29) Score(109.70,94.96) Score(79.80,90.80) Score(99.71,106.05) Score(97.05,111.33) Score(122.20,113.22) Score(159.28,115.53) Score(146.35,119.66) Score(140.28,114.22) Score(100.09,101.22) Score(98.31,97.77) Score(86.05,107.36) Score(141.31,97.74) Score(145.67,102.42) Score(143.09,97.14) Score(130.95,94.43) Score(159.88,112.36) Score(138.60,98.76) Score(105.34,109.80) Score(62.40,84.10) Score(118.62,94.73) Score(85.08,112.00) Score(89.89,92.40) Score(130.12,87.90) Score(152.90,84.50) Score(121.20,98.04) Score(54.80,77.25) Score(81.95,94.28) Score(106.57,87.08) Score(25.30,99.09) Score(69.79,101.14) Score(81.16,101.78) Score(61.06,93.96) Score(68.15,70.06) Score(99.69,83.88) Score(92.02,114.97) Score(40.49,96.76) Score(78.83,101.88) Score(97.75,78.91) Score(103.16,71.36) Score(99.47,88.49) Score(42.26,96.57) Score(25.09,110.69) Score(-41.21,99.54) Score(-24.19,99.99) Score(-6.75,109.07) Score(-1.85,89.89) Score(10.30,73.52) Score(38.73,73.95) Score(82.77,72.57) Score(-37.22,86.87) Score(-10.98,114.63) Score(-54.81,120.14) Score(7.18,106.42) Score(-14.16,103.68) Score(-12.34,82.22) Score(26.17,89.11) Score(107.39,40.10) Score(78.17,79.52) Score(52.81,113.36) Score(63.07,113.18) Score(61.33,110.09) Score(72.01,96.74) Score(60.13,88.72) Score(30.34,103.56) Score(105.09,94.27) Score(79.69,85.39) Score(82.07,116.01) Score(86.38,111.12) Score(108.27,120.36) Score(125.88,102.58) Score(120.72,99.55) Score(135.09,106.48) Score(101.85,109.30) Score(88.91,111.29) Score(101.33,118.17) Score(126.04,117.66) Score(128.60,114.38) Score(135.91,108.32) Score(121.47,110.60) Score(152.69,104.72) Score(171.42,106.35) Score(125.35,106.81) Score(131.58,121.75) Score(133.15,123.32) Score(158.34,121.87) Score(164.75,116.07) Score(176.22,101.34) Score(173.84,112.78) Score(131.19,118.48) Score(151.85,112.89) Score(131.79,123.87) Score(137.31,120.10) Score(132.58,126.68) Score(144.12,119.19) Score(150.82,114.67) Score(112.00,122.85) Score(134.09,118.78) Score(150.49,119.96) Score(-73.15,39.30) Score(-61.72,50.92) Score(-41.40,52.58) Score(97.55,70.40) Score(-26.17,71.84) Score(-94.65,16.65) Score(-8.78,64.42) Score(-65.47,64.20) Score(26.50,102.77) Score(22.85,91.39) Score(93.75,91.85) Score(84.47,107.92) Score(68.10,95.04) Score(65.29,75.84) Score(60.44,67.63) Score(69.00,73.10) Score(36.40,109.24) Score(81.29,94.03) Score(77.97,135.77) Score(55.33,126.78) Score(59.18,129.90) Score(109.12,137.68) Score(130.11,136.96) Score(103.25,131.50) Score(51.35,107.96) Score(84.96,146.55) Score(80.94,134.93) Score(96.79,161.92) Score(109.11,147.62) Score(108.00,154.50) Score(118.83,160.51) Score(119.53,147.02) Score(88.24,123.87) Score(81.11,140.34) Score(97.05,141.81) Score(95.03,160.58) Score(111.07,174.16) Score(132.34,158.63) Score(116.25,179.19) Score(117.63,155.43) Score(104.57,99.30) Score(92.16,124.32) Score(118.21,126.30) Score(118.22,166.36) Score(137.50,162.17) Score(172.78,153.65) Score(160.98,136.56) Score(172.74,128.66) Score(93.65,103.94) Score(70.04,138.36) Score(107.23,150.99) Score(112.77,158.83) Score(99.48,176.57) Score(170.48,144.28) Score(140.53,148.11) Score(171.20,119.31) Score(88.60,110.78) Score(115.57,142.51) Score(145.67,142.06) Score(128.76,146.67) Score(175.23,146.60) Score(161.84,139.33) Score(161.42,130.36) Score(163.55,141.45) Score(60.09,-88.44) Score(147.98,-80.36) Score(74.49,-38.08) Score(-140.60,-51.52) Score(-24.20,-16.59) Score(-125.69,-44.58) Score(97.64,-58.23) Score(43.10,-106.67) Score(116.78,-35.52) Score(-4.58,-2.65) Score(-73.33,14.74) Score(-153.06,36.74) Score(-143.73,40.81) Score(-111.64,43.76) Score(-11.39,7.02) Score(47.31,-66.35) Score(-31.84,-74.86) Score(-69.92,2.82) Score(-115.05,40.73) Score(-153.12,68.75) Score(-142.90,79.90) Score(-136.91,37.59) Score(-101.84,10.06) Score(-111.26,-19.28) Score(-101.45,-34.99) Score(-84.67,33.27) Score(-109.30,95.45) Score(-112.51,125.37) Score(-123.03,112.04) Score(-157.95,96.22) Score(-140.77,51.65) Score(-145.59,0.39) Score(-81.92,-26.17) Score(-75.57,80.38) Score(-58.90,111.54) Score(-84.41,124.68) Score(-80.46,127.48) Score(-85.80,126.50) Score(-83.77,104.16) Score(-137.21,35.20) Score(-82.47,-3.94) Score(108.71,47.71) Score(-69.24,86.60) Score(-59.79,97.62) Score(-48.37,106.98) Score(34.36,136.50) Score(45.77,122.10) Score(-82.93,34.67) Score(-40.22,-67.47) Score(-4.03,50.27) Score(-114.44,45.44) Score(-85.07,16.57) Score(-79.94,60.32) Score(-81.94,92.44) Score(-17.93,93.46) Score(-62.02,26.33) Score(-109.10,-159.51) Score(-47.33,-94.64) Score(-59.57,-104.33) Score(-102.69,-30.82) Score(-151.20,-97.78) Score(-118.62,-18.44) Score(-28.09,10.21) Score(-40.33,-109.22)
"""

fixed_scores = fix_linebreaks(scores)

# Parsing the parameters
parameters = parse_parameter_definitions(cpp_code)
result = extract_and_assign_scores(fixed_scores, parameters)

# Output result
# print(parameters)

for param_name, score_str in result.items():
    print(score_str)
